import { readFile } from 'node:fs/promises';

import { parseColor, type Rgba } from './color.js';
import { decodePng } from './png.js';
import { assetPath } from './paths.js';

/**
 * Load a master palette from either an explicit hex list or a file.
 *
 * Supported files:
 *   .gpl        GIMP palette (what LibreSprite exports)
 *   .png/.pal   an image; every distinct opaque colour becomes an entry, which
 *               is how palette swatch strips are usually shared
 *   .hex/.txt   one hex colour per line
 */
export async function loadPalette(opts: {
  colors?: string[];
  file?: string;
}): Promise<Rgba[]> {
  if (opts.colors && opts.colors.length > 0) {
    return opts.colors.map(parseColor);
  }
  if (!opts.file) {
    throw new Error('Provide either `palette` (hex colours) or `paletteFile`.');
  }

  const target = assetPath(opts.file);
  const lower = target.host.toLowerCase();

  if (lower.endsWith('.png')) {
    const img = decodePng(await readFile(target.host));
    const seen = new Map<number, Rgba>();
    for (let i = 0; i < img.data.length; i += 4) {
      const a = img.data[i + 3]!;
      if (a === 0) continue;
      const r = img.data[i]!, g = img.data[i + 1]!, b = img.data[i + 2]!;
      const key = (r << 16) | (g << 8) | b;
      if (!seen.has(key)) seen.set(key, { r, g, b, a: 255 });
    }
    if (seen.size === 0) throw new Error(`Palette image ${target.rel} has no opaque pixels.`);
    return [...seen.values()];
  }

  const text = await readFile(target.host, 'utf8');

  if (lower.endsWith('.gpl')) {
    const out: Rgba[] = [];
    for (const raw of text.split('\n')) {
      const line = raw.trim();
      if (!line || line.startsWith('#')) continue;
      if (/^(GIMP Palette|Name:|Columns:)/i.test(line)) continue;
      const m = /^(\d+)\s+(\d+)\s+(\d+)/.exec(line);
      if (m) out.push({ r: +m[1]!, g: +m[2]!, b: +m[3]!, a: 255 });
    }
    if (out.length === 0) throw new Error(`No colours parsed from ${target.rel}.`);
    return out;
  }

  const out = text
    .split(/\s+/)
    .map((s) => s.trim())
    .filter((s) => /^#?[0-9a-f]{3,8}$/i.test(s))
    .map(parseColor);
  if (out.length === 0) {
    throw new Error(`Could not parse a palette from ${target.rel}. Use .gpl, .png, or hex per line.`);
  }
  return out;
}
