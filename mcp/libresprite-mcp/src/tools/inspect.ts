import type { McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';

import { runNative, runScript } from '../docker.js';
import { assetPath } from '../paths.js';
import { guard, json, type ToolResult } from '../result.js';

const FileArg = z
  .string()
  .describe('Path to a .ase/.aseprite/.png file, relative to the assets root.');

interface SpriteInfo {
  file: string;
  width: number;
  height: number;
  colorMode: string;
  layerCount: number;
  frameCount: number;
  paletteSize: number;
  layers: Array<{
    index: number;
    name: string;
    visible: boolean;
    editable: boolean;
    isImage: boolean;
    isBackground: boolean;
    celCount: number;
  }>;
}

interface Preview {
  png: string;
  width: number;
  height: number;
  x: number;
  y: number;
  layer: string;
  layerCount: number;
  frame: number;
}

/**
 * LibreSprite logs its own progress to stdout alongside the names that
 * --list-layers/--list-tags print, e.g.
 *   Reading file "/scripts/op.js"
 *   Loading file "/work/test/hero.png" (png)
 *   Using loader for format png
 * Strip those so callers get names only.
 */
const LOG_NOISE = /^(Reading file|Loading file|Using loader|Saving file|Writing file|Error|Warning)\b/i;

function lines(stdout: string): string[] {
  return stdout
    .split('\n')
    .map((l) => l.trim())
    .filter((l) => l.length > 0 && !LOG_NOISE.test(l));
}

export function registerInspectTools(server: McpServer): void {
  server.registerTool(
    'sprite_info',
    {
      description:
        'Full metadata for a sprite: dimensions, colour mode, frame count, palette size and every ' +
        'layer (including hidden ones). Start here before editing or exporting anything.',
      inputSchema: z.object({ file: FileArg }),
    },
    guard(async ({ file }: { file: string }): Promise<ToolResult> => {
      const target = assetPath(file);
      const info = await runScript<SpriteInfo>('sprite_info', { file: target.container });
      return json({ ...info, file: target.rel });
    }),
  );

  server.registerTool(
    'list_layers',
    {
      description:
        'Layer names via the native CLI. Faster than sprite_info but lists only VISIBLE layers ' +
        'unless allLayers is set. Use sprite_info when you need indices or visibility state.',
      inputSchema: z.object({
        file: FileArg,
        allLayers: z
          .boolean()
          .default(false)
          .describe('Force every layer visible first, so hidden layers are listed too.'),
      }),
    },
    guard(async ({ file, allLayers }: { file: string; allLayers: boolean }): Promise<ToolResult> => {
      const target = assetPath(file);
      const args = ['--list-layers'];
      if (allLayers) args.unshift('--all-layers');
      const { stdout } = await runNative([...args, target.container]);
      return json({ file: target.rel, layers: lines(stdout) });
    }),
  );

  server.registerTool(
    'list_tags',
    {
      description:
        'Animation tag names defined in the sprite. Tags are the unit of animation export ' +
        '(export_spritesheet frameTag), and the script API cannot reach them, so this is the only route.',
      inputSchema: z.object({ file: FileArg }),
    },
    guard(async ({ file }: { file: string }): Promise<ToolResult> => {
      const target = assetPath(file);
      const { stdout } = await runNative(['--list-tags', target.container]);
      return json({ file: target.rel, tags: lines(stdout) });
    }),
  );

  server.registerTool(
    'render_preview',
    {
      description:
        'Render ONE layer of a frame to PNG and return it as an image, so you can actually look ' +
        'at the art before and after editing it. Headless LibreSprite cannot composite layers, ' +
        'so on a multi-layer sprite this shows a single layer, not the finished picture — the ' +
        'result says which one. For a true composite, export via export_png. Never writes.',
      inputSchema: z.object({
        file: FileArg,
        frame: z.number().int().min(0).default(0),
        layer: z
          .union([z.string(), z.number().int().min(0)])
          .optional()
          .describe('Layer name or index. Omit to use the first visible image layer.'),
      }),
    },
    guard(
      async ({
        file,
        frame,
        layer,
      }: {
        file: string;
        frame: number;
        layer?: string | number;
      }): Promise<ToolResult> => {
        const target = assetPath(file);
        const out = await runScript<Preview>('render_preview', {
          file: target.container,
          frame,
          layer: layer ?? null,
        });
        const label =
          `${target.rel} frame ${out.frame}, layer "${out.layer}" ` +
          `(${out.width}x${out.height} at ${out.x},${out.y})` +
          (out.layerCount > 1
            ? ` — showing 1 of ${out.layerCount} layers, NOT the composite`
            : '');
        return {
          content: [
            { type: 'image', data: out.png, mimeType: 'image/png' },
            { type: 'text', text: label },
          ],
        };
      },
    ),
  );
}
