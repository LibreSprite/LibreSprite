import { readFile } from 'node:fs/promises';

import type { McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';

import { runNative, withFileLock } from '../docker.js';
import { assetPath } from '../paths.js';
import { guard, json, type ToolResult } from '../result.js';

/**
 * LibreSprite consumes options and filenames positionally
 * (src/app/app.cpp:214-639), and --save-as acts on documents().lastAdded()
 * (app.cpp:355-360). So argv is always assembled in this order:
 *
 *   1. modifier flags that only set variables
 *   2. the input file
 *   3. flags that need an open document (--scale)
 *   4. the output flag (--save-as)
 *
 * Never concatenate model-supplied strings outside this shape.
 */

export function registerExportTools(server: McpServer): void {
  server.registerTool(
    'export_png',
    {
      description:
        'Export a sprite to PNG (or any format LibreSprite writes) through the native CLI. ' +
        'Supports scaling, layer isolation, frame ranges, trimming and cropping.',
      inputSchema: z.object({
        file: z.string().describe('Input sprite, relative to the assets root.'),
        out: z
          .string()
          .describe(
            'Output path relative to the assets root. With splitLayers or multiple frames, use ' +
              'filenameFormat tokens: {path} {title} {layer} {frame} {extension}.',
          ),
        scale: z.number().positive().optional().describe('Integer scale is best for pixel art.'),
        layer: z.string().optional().describe('Export only this layer, by name.'),
        allLayers: z.boolean().default(false).describe('Include hidden layers.'),
        splitLayers: z.boolean().default(false).describe('One file per layer.'),
        frameRange: z
          .string()
          .regex(/^\d+,\d+$/)
          .optional()
          .describe('Inclusive frame range, e.g. "0,7".'),
        crop: z
          .string()
          .regex(/^\d+,\d+,\d+,\d+$/)
          .optional()
          .describe('x,y,width,height.'),
        trim: z.boolean().default(false).describe('Autocrop empty borders before saving.'),
        filenameFormat: z.string().optional(),
      }),
    },
    guard(async (a): Promise<ToolResult> => {
      const input = assetPath(a.file);
      const output = assetPath(a.out);

      const pre: string[] = [];
      if (a.allLayers) pre.push('--all-layers');
      if (a.layer) pre.push('--layer', a.layer);
      if (a.splitLayers) pre.push('--split-layers');
      if (a.frameRange) pre.push('--frame-range', a.frameRange);
      if (a.crop) pre.push('--crop', a.crop);
      if (a.trim) pre.push('--trim');
      if (a.filenameFormat) pre.push('--filename-format', a.filenameFormat);

      const post: string[] = [];
      if (a.scale !== undefined) post.push('--scale', String(a.scale));

      const { stdout, stderr } = await withFileLock(output.host, () =>
        runNative([...pre, input.container, ...post, '--save-as', output.container]),
      );

      return json({
        input: input.rel,
        output: output.rel,
        log: [stdout.trim(), stderr.trim()].filter(Boolean).join('\n') || undefined,
      });
    }),
  );

  server.registerTool(
    'resize_sprite',
    {
      description:
        'Scale, crop or trim a sprite into a NEW file via the native CLI. This is the only route ' +
        'that works: the script API hangs on resize. Never overwrites the source — pass a ' +
        'distinct `out`. Use integer scale factors for pixel art.',
      inputSchema: z.object({
        file: z.string(),
        out: z.string().describe('Destination path. Must differ from `file`.'),
        scale: z.number().positive().optional().describe('e.g. 2 doubles the sprite.'),
        crop: z
          .string()
          .regex(/^\d+,\d+,\d+,\d+$/)
          .optional()
          .describe('x,y,width,height.'),
        trim: z.boolean().default(false).describe('Autocrop empty borders.'),
      }),
    },
    guard(async (a): Promise<ToolResult> => {
      const input = assetPath(a.file);
      const output = assetPath(a.out);
      if (input.host === output.host) {
        throw new Error('`out` must differ from `file`; this tool will not overwrite the source.');
      }
      if (a.scale === undefined && !a.crop && !a.trim) {
        throw new Error('Nothing to do: pass at least one of scale, crop or trim.');
      }

      const pre: string[] = [];
      if (a.crop) pre.push('--crop', a.crop);
      if (a.trim) pre.push('--trim');

      // --scale needs an open document, so it must follow the input file.
      const post: string[] = [];
      if (a.scale !== undefined) post.push('--scale', String(a.scale));

      await withFileLock(output.host, () =>
        runNative([...pre, input.container, ...post, '--save-as', output.container]),
      );
      return json({ input: input.rel, output: output.rel, scale: a.scale, crop: a.crop, trim: a.trim });
    }),
  );

  server.registerTool(
    'export_spritesheet',
    {
      description:
        'Pack frames into a texture atlas plus a JSON descriptor. This is the handoff format for ' +
        'the game engine. The parsed atlas JSON comes back in the result, so you can check frame ' +
        'rects and tags without reading the file separately.',
      inputSchema: z.object({
        file: z.string(),
        sheet: z.string().describe('Output PNG for the atlas, relative to the assets root.'),
        data: z
          .string()
          .optional()
          .describe('Output JSON descriptor. Strongly recommended; without it you get pixels only.'),
        type: z
          .enum(['horizontal', 'vertical', 'rows', 'columns', 'packed'])
          .default('packed')
          .describe('"packed" uses best-fit bin packing; "rows" keeps a readable grid.'),
        format: z.enum(['json-hash', 'json-array']).default('json-array'),
        sheetWidth: z.number().int().positive().optional(),
        sheetHeight: z.number().int().positive().optional(),
        splitLayers: z.boolean().default(false),
        allLayers: z.boolean().default(false),
        layer: z.string().optional(),
        frameTag: z.string().optional().describe('Export only the frames under this animation tag.'),
        frameRange: z.string().regex(/^\d+,\d+$/).optional(),
        trim: z.boolean().default(false),
        ignoreEmpty: z.boolean().default(true),
        borderPadding: z.number().int().min(0).default(0),
        shapePadding: z.number().int().min(0).default(0),
        innerPadding: z.number().int().min(0).default(0),
        listLayers: z.boolean().default(true).describe('Add a meta.layers block to the JSON.'),
        listTags: z.boolean().default(true).describe('Add a meta.frameTags block to the JSON.'),
      }),
    },
    guard(async (a): Promise<ToolResult> => {
      const input = assetPath(a.file);
      const sheet = assetPath(a.sheet);
      const data = a.data ? assetPath(a.data) : undefined;

      const args: string[] = ['--sheet', sheet.container, '--sheet-type', a.type];
      if (a.sheetWidth) args.push('--sheet-width', String(a.sheetWidth));
      if (a.sheetHeight) args.push('--sheet-height', String(a.sheetHeight));
      if (data) args.push('--data', data.container, '--format', a.format);
      if (a.allLayers) args.push('--all-layers');
      if (a.layer) args.push('--layer', a.layer);
      if (a.splitLayers) args.push('--split-layers');
      if (a.frameTag) args.push('--frame-tag', a.frameTag);
      if (a.frameRange) args.push('--frame-range', a.frameRange);
      if (a.trim) args.push('--trim');
      if (a.ignoreEmpty) args.push('--ignore-empty');
      if (a.borderPadding) args.push('--border-padding', String(a.borderPadding));
      if (a.shapePadding) args.push('--shape-padding', String(a.shapePadding));
      if (a.innerPadding) args.push('--inner-padding', String(a.innerPadding));
      if (a.listLayers) args.push('--list-layers');
      if (a.listTags) args.push('--list-tags');

      await withFileLock(sheet.host, () => runNative([...args, input.container]));

      let atlas: unknown;
      if (data) {
        try {
          atlas = JSON.parse(await readFile(data.host, 'utf8'));
        } catch (err) {
          throw new Error(
            `Sheet was written but the JSON at ${data.rel} could not be read: ` +
              (err instanceof Error ? err.message : String(err)),
          );
        }
      }

      return json({
        input: input.rel,
        sheet: sheet.rel,
        data: data?.rel,
        type: a.type,
        atlas,
      });
    }),
  );
}
