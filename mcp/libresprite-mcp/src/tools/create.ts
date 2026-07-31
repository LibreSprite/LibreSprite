import { mkdir, readFile, rm, writeFile } from 'node:fs/promises';
import { dirname } from 'node:path';

import type { McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';

import { parseColor } from '../color.js';
import { runNative, withFileLock } from '../docker.js';
import { loadPalette } from '../palette.js';
import { assetPath, tempAsset, type AssetPath } from '../paths.js';
import { blankRaster, decodePng, encodePng } from '../png.js';
import { countColors, quantize, resize } from '../raster.js';
import { guard, json, type ToolResult } from '../result.js';

/**
 * Pixel work happens on the host; LibreSprite is used for what only it can do,
 * which is reading and writing .ase. Round-tripping a 256x256 image through
 * Duktape one getPixel() at a time would take ~20s, so these tools decode PNG
 * here and use the container purely as a format converter.
 */

async function writeRasterAs(
  raster: { width: number; height: number; data: Uint8Array },
  out: AssetPath,
): Promise<void> {
  const tmp = tempAsset('.png');
  await mkdir(dirname(tmp.host), { recursive: true });
  await mkdir(dirname(out.host), { recursive: true });
  try {
    await writeFile(tmp.host, encodePng(raster));
    if (out.host.toLowerCase().endsWith('.png')) {
      await writeFile(out.host, encodePng(raster));
      return;
    }
    // Let LibreSprite produce the .ase (or whatever the extension asks for).
    await runNative([tmp.container, '--save-as', out.container]);
  } finally {
    await rm(dirname(tmp.host), { recursive: true, force: true });
  }
}

/** Get a source sprite's pixels, converting through PNG when needed. */
async function readSourceRaster(src: AssetPath) {
  if (src.host.toLowerCase().endsWith('.png')) {
    return decodePng(await readFile(src.host));
  }
  const tmp = tempAsset('.png');
  await mkdir(dirname(tmp.host), { recursive: true });
  try {
    await runNative([src.container, '--save-as', tmp.container]);
    return decodePng(await readFile(tmp.host));
  } finally {
    await rm(dirname(tmp.host), { recursive: true, force: true });
  }
}

export function registerCreateTools(server: McpServer): void {
  server.registerTool(
    'create_sprite',
    {
      description:
        'Create a new, empty sprite file (.ase or .png) of a given size. Transparent by default. ' +
        'Use this to start a sprite from scratch, then draw into it with pixel_ops.',
      inputSchema: z.object({
        out: z.string().describe('Destination path relative to the assets root, e.g. "chars/hero.ase".'),
        width: z.number().int().positive().max(4096),
        height: z.number().int().positive().max(4096),
        background: z
          .string()
          .optional()
          .describe('Hex fill colour. Omit for a fully transparent canvas.'),
        overwrite: z.boolean().default(false),
      }),
    },
    guard(async (a): Promise<ToolResult> => {
      const out = assetPath(a.out);
      if (!a.overwrite) {
        const exists = await readFile(out.host).then(
          () => true,
          () => false,
        );
        if (exists) throw new Error(`${out.rel} already exists. Pass overwrite: true to replace it.`);
      }
      const bg = a.background ? parseColor(a.background) : { r: 0, g: 0, b: 0, a: 0 };
      const raster = blankRaster(a.width, a.height, [bg.r, bg.g, bg.b, bg.a]);
      await withFileLock(out.host, () => writeRasterAs(raster, out));
      return json({ output: out.rel, width: a.width, height: a.height, background: a.background ?? 'transparent' });
    }),
  );

  server.registerTool(
    'convert_to_pixel_art',
    {
      description:
        'Convert an ordinary drawing or photo into a small, palette-limited sprite: resample down, ' +
        'then snap every colour to a master palette. Writes a NEW file and never touches the source. ' +
        'This produces a mechanically correct sprite, not hand-quality pixel art — treat the result ' +
        'as a draft or colour blockout, not a finished asset.',
      inputSchema: z.object({
        file: z.string().describe('Source image (.png, .ase, ...), relative to the assets root.'),
        out: z.string().describe('Destination, e.g. "chars/hero.ase". Must differ from the source.'),
        width: z.number().int().positive().max(1024).optional(),
        height: z.number().int().positive().max(1024).optional(),
        scale: z
          .number()
          .positive()
          .optional()
          .describe('Alternative to width/height, e.g. 0.125 to go from 256 to 32.'),
        resample: z
          .enum(['area', 'nearest'])
          .default('area')
          .describe(
            'area averages each source block — right for drawings and photos. ' +
              'nearest samples one pixel per block — right for rescaling art that is ALREADY ' +
              'pixel art, since it never invents a colour.',
          ),
        palette: z.array(z.string()).optional().describe('Master palette as hex colours.'),
        paletteFile: z
          .string()
          .optional()
          .describe('Palette file instead: .gpl, a .png swatch strip, or hex per line.'),
        alphaThreshold: z
          .number()
          .int()
          .min(0)
          .max(255)
          .default(128)
          .describe('Pixels below this alpha become fully transparent. Kills soft edges.'),
      }),
    },
    guard(async (a): Promise<ToolResult> => {
      const src = assetPath(a.file);
      const out = assetPath(a.out);
      if (src.host === out.host) {
        throw new Error('`out` must differ from `file`; this tool will not overwrite the source.');
      }

      const source = await readSourceRaster(src);

      let width = a.width ?? 0;
      let height = a.height ?? 0;
      if (a.scale !== undefined) {
        width = Math.max(1, Math.round(source.width * a.scale));
        height = Math.max(1, Math.round(source.height * a.scale));
      } else if (width && !height) {
        height = Math.max(1, Math.round((source.height * width) / source.width));
      } else if (height && !width) {
        width = Math.max(1, Math.round((source.width * height) / source.height));
      }
      if (!width || !height) {
        throw new Error('Give a target size: width, height, both, or scale.');
      }

      const resized = resize(source, width, height, a.resample);

      let final = resized;
      let paletteSize: number | undefined;
      if (a.palette?.length || a.paletteFile) {
        const pal = await loadPalette({ colors: a.palette, file: a.paletteFile });
        paletteSize = pal.length;
        final = quantize(resized, pal, { alphaThreshold: a.alphaThreshold }).raster;
      }

      await withFileLock(out.host, () => writeRasterAs(final, out));

      return json({
        input: src.rel,
        output: out.rel,
        source: { width: source.width, height: source.height, colors: countColors(source) },
        result: { width, height, colors: countColors(final) },
        resample: a.resample,
        paletteSize,
        note: paletteSize
          ? undefined
          : 'No palette given, so colours were only resampled, not reduced. Pass palette or paletteFile to limit the colour count.',
      });
    }),
  );
}
