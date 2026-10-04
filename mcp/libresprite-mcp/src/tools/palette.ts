import type { McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';

import { normalizeColorKey } from '../color.js';
import { runScript, withFileLock } from '../docker.js';
import { loadPalette } from '../palette.js';
import { assetPath } from '../paths.js';
import { guard, json, type ToolResult } from '../result.js';

interface PaletteOut {
  size: number;
  colorMode: string;
  colors: string[];
}

interface ValidationOut {
  file: string;
  colorMode: string;
  width: number;
  height: number;
  paletteSize: number;
  uniqueColors: number;
  pixelsScanned: number;
  truncated: boolean;
  valid: boolean;
  problems: string[];
  offendingColors: Array<{ color: string; layer: string; frame: number; x: number; y: number }>;
  offendingColorCount: number;
  usedColors: Array<{ color: string; count: number }>;
}

export function registerPaletteTools(server: McpServer): void {
  server.registerTool(
    'get_palette',
    {
      description: "Read the sprite's palette as hex colours.",
      inputSchema: z.object({ file: z.string() }),
    },
    guard(async ({ file }: { file: string }): Promise<ToolResult> => {
      const target = assetPath(file);
      const out = await runScript<PaletteOut>('get_palette', { file: target.container });
      return json({ file: target.rel, ...out });
    }),
  );

  server.registerTool(
    'validate_palette',
    {
      description:
        'The gatekeeper for the art style. Scans every pixel of every cel and reports colours ' +
        'outside the master palette, the total unique colour count, and whether the canvas fits ' +
        'the tile grid. Run this before accepting any sprite into the project. Read-only.',
      inputSchema: z.object({
        file: z.string(),
        allowed: z
          .array(z.string())
          .optional()
          .describe(
            'Master palette as hex colours. Any pixel colour not in this list is reported as a violation.',
          ),
        maxColors: z
          .number()
          .int()
          .positive()
          .optional()
          .describe('Fail if the sprite uses more unique colours than this.'),
        tileWidth: z.number().int().positive().optional(),
        tileHeight: z
          .number()
          .int()
          .positive()
          .optional()
          .describe('Fail if the canvas is not a whole number of tiles.'),
        ignoreTransparent: z
          .boolean()
          .default(true)
          .describe('Skip fully transparent pixels (alpha 0).'),
        visibleOnly: z.boolean().default(false).describe('Only scan visible layers.'),
        maxReported: z.number().int().positive().max(500).default(50),
        maxPixels: z
          .number()
          .int()
          .positive()
          .default(4_000_000)
          .describe('Scan budget. If exceeded the result is marked truncated.'),
      }),
    },
    guard(async (a): Promise<ToolResult> => {
      const target = assetPath(a.file);
      const out = await runScript<ValidationOut>('validate_palette', {
        file: target.container,
        allowed: a.allowed ? a.allowed.map(normalizeColorKey) : null,
        maxColors: a.maxColors ?? null,
        tileWidth: a.tileWidth ?? null,
        tileHeight: a.tileHeight ?? null,
        ignoreTransparent: a.ignoreTransparent,
        visibleOnly: a.visibleOnly,
        maxReported: a.maxReported,
        maxPixels: a.maxPixels,
      });
      return json({ ...out, file: target.rel });
    }),
  );

  server.registerTool(
    'quantize_to_palette',
    {
      description:
        'Snap every pixel of a sprite to its nearest master-palette colour, IN PLACE, preserving ' +
        'layers and frames. This is how you make an off-palette sprite conform. It does not change ' +
        'the colour mode (that is impossible headless) — the sprite stays RGB but uses only ' +
        'palette colours, which is what validate_palette checks. There is no undo: it rewrites ' +
        'the file. Run validate_palette first and render_preview after.',
      inputSchema: z.object({
        file: z.string(),
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
          .describe('Pixels below this alpha become fully transparent.'),
        maxPixels: z.number().int().positive().default(1_000_000),
      }),
    },
    guard(async (a): Promise<ToolResult> => {
      const target = assetPath(a.file);
      const pal = await loadPalette({ colors: a.palette, file: a.paletteFile });

      // Only opaque entries are valid match targets. A transparent entry like
      // #00000000 would otherwise reach the script as {r:0,g:0,b:0} and pull
      // dark pixels onto an opaque black that is not in the palette at all.
      // Mirrors the same filter in raster.ts quantize().
      const opaque = pal.filter((c) => c.a !== 0);
      if (opaque.length === 0) {
        throw new Error('Palette contains no opaque colours; nothing to quantise to.');
      }

      const out = await withFileLock(target.host, () =>
        runScript<Record<string, unknown>>('quantize', {
          file: target.container,
          palette: opaque.map((c) => ({ r: c.r, g: c.g, b: c.b })),
          alphaThreshold: a.alphaThreshold,
          maxPixels: a.maxPixels,
        }),
      );
      return json({ ...out, file: target.rel });
    }),
  );
}
