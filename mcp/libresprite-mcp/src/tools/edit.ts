import type { McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';

import { parseColor } from '../color.js';
import { runScript, withFileLock } from '../docker.js';
import { assetPath } from '../paths.js';
import { guard, json, type ToolResult } from '../result.js';

const LayerRef = z
  .union([z.string(), z.number().int().min(0)])
  .describe('Layer name, or index counting from 0 at the bottom.');

/** Colour as hex, or a raw palette index for indexed sprites. */
const Color = z.union([
  z.string().describe('Hex colour: #rgb, #rgba, #rrggbb or #rrggbbaa.'),
  z.object({ index: z.number().int().min(0) }).describe('Palette index, for indexed sprites.'),
]);

type ColorInput = z.infer<typeof Color>;

function encodeColor(c: ColorInput): unknown {
  return typeof c === 'string' ? parseColor(c) : c;
}

/**
 * Only property writes. Structural edits (add/remove/merge/flatten) run through
 * app::Command, which headless LibreSprite refuses -- verified against the
 * built container, see the capability matrix in templates.ts.
 */
const LayerOp = z.discriminatedUnion('op', [
  z.object({ op: z.literal('set_visible'), layer: LayerRef, value: z.boolean() }),
  z.object({ op: z.literal('set_editable'), layer: LayerRef, value: z.boolean() }),
  z.object({ op: z.literal('rename'), layer: LayerRef, name: z.string().min(1) }),
]);

const PixelOp = z.discriminatedUnion('op', [
  z.object({
    op: z.literal('put_pixels'),
    pixels: z
      .array(
        z.object({
          x: z.number().int(),
          y: z.number().int(),
          value: Color,
        }),
      )
      .min(1)
      .max(100_000),
  }),
  z.object({
    op: z.literal('fill_rect'),
    x: z.number().int(),
    y: z.number().int(),
    w: z.number().int().positive(),
    h: z.number().int().positive(),
    value: Color,
  }),
  z.object({
    op: z.literal('replace_color'),
    from: Color,
    to: Color,
  }),
  z.object({ op: z.literal('clear'), value: Color }),
]);

export function registerEditTools(server: McpServer): void {
  server.registerTool(
    'layer_ops',
    {
      description:
        'Rename layers or toggle their visible/editable flags, then save the sprite. ' +
        'Adding, removing, merging and flattening layers are NOT possible headless and are ' +
        'rejected by the schema -- those need the GUI (docker compose --profile gui up).',
      inputSchema: z.object({
        file: z.string(),
        ops: z.array(LayerOp).min(1),
      }),
    },
    guard(async (a): Promise<ToolResult> => {
      const target = assetPath(a.file);
      const out = await withFileLock(target.host, () =>
        runScript<Record<string, unknown>>('layer_ops', {
          file: target.container,
          ops: a.ops,
        }),
      );
      return json({ ...out, file: target.rel });
    }),
  );

  server.registerTool(
    'pixel_ops',
    {
      description:
        'Draw into one cel (a layer at a frame) and save the sprite. Writes go through putPixel, ' +
        'so keep batches to the thousands rather than millions of pixels. Call render_preview ' +
        'afterwards to check the result.',
      inputSchema: z.object({
        file: z.string(),
        layer: LayerRef,
        frame: z.number().int().min(0).default(0),
        ops: z.array(PixelOp).min(1),
        allowIndexed: z
          .boolean()
          .default(false)
          .describe(
            'Required for INDEXED sprites, where pixel values are palette indices rather than colours.',
          ),
      }),
    },
    guard(async (a): Promise<ToolResult> => {
      const target = assetPath(a.file);

      // Colour components are resolved engine-side; see lsColorValue in the prelude.
      const ops = a.ops.map((op) => {
        switch (op.op) {
          case 'put_pixels':
            return {
              ...op,
              pixels: op.pixels.map((p) => ({ ...p, value: encodeColor(p.value) })),
            };
          case 'fill_rect':
          case 'clear':
            return { ...op, value: encodeColor(op.value) };
          case 'replace_color':
            return { ...op, from: encodeColor(op.from), to: encodeColor(op.to) };
        }
      });

      const out = await withFileLock(target.host, () =>
        runScript<Record<string, unknown>>('pixel_ops', {
          file: target.container,
          layer: a.layer,
          frame: a.frame,
          ops,
          allowIndexed: a.allowIndexed,
        }),
      );
      return json({ ...out, file: target.rel });
    }),
  );
}
