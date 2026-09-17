import type { Rgba } from './color.js';
import type { Raster } from './png.js';

/**
 * Host-side resampling and quantisation.
 *
 * Two resampling modes, because they serve opposite jobs:
 *
 *   nearest  Samples one source pixel per destination pixel. Correct for
 *            rescaling art that is ALREADY pixel art -- it never invents a
 *            colour, so the palette survives. This matches LibreSprite's own
 *            --scale (RESIZE_METHOD_NEAREST_NEIGHBOR, cmd_sprite_size.cpp:258).
 *            On a smooth drawing it produces noise: each destination pixel is
 *            one arbitrary sample from a block it does not represent.
 *
 *   area     Averages the whole source block. Much better for converting a
 *            drawing or photo, because it preserves shape and mass. It does
 *            invent colours, which is exactly why quantisation runs afterwards.
 */
export type Resample = 'area' | 'nearest';

export function resize(src: Raster, width: number, height: number, mode: Resample): Raster {
  if (width === src.width && height === src.height) {
    return { width, height, data: src.data.slice() };
  }
  return mode === 'nearest' ? resizeNearest(src, width, height) : resizeArea(src, width, height);
}

function resizeNearest(src: Raster, width: number, height: number): Raster {
  const out = new Uint8Array(width * height * 4);
  for (let y = 0; y < height; y++) {
    const sy = Math.min(src.height - 1, Math.floor((y * src.height) / height));
    for (let x = 0; x < width; x++) {
      const sx = Math.min(src.width - 1, Math.floor((x * src.width) / width));
      const s = (sy * src.width + sx) * 4;
      const d = (y * width + x) * 4;
      out[d] = src.data[s]!;
      out[d + 1] = src.data[s + 1]!;
      out[d + 2] = src.data[s + 2]!;
      out[d + 3] = src.data[s + 3]!;
    }
  }
  return { width, height, data: out };
}

function resizeArea(src: Raster, width: number, height: number): Raster {
  const out = new Uint8Array(width * height * 4);
  const xRatio = src.width / width;
  const yRatio = src.height / height;

  for (let y = 0; y < height; y++) {
    const y0 = Math.floor(y * yRatio);
    const y1 = Math.max(y0 + 1, Math.min(src.height, Math.ceil((y + 1) * yRatio)));
    for (let x = 0; x < width; x++) {
      const x0 = Math.floor(x * xRatio);
      const x1 = Math.max(x0 + 1, Math.min(src.width, Math.ceil((x + 1) * xRatio)));

      // Colour is averaged weighted by alpha (premultiplied) so transparent
      // pixels do not drag a dark halo into the edges.
      let r = 0, g = 0, b = 0, a = 0, n = 0;
      for (let sy = y0; sy < y1; sy++) {
        for (let sx = x0; sx < x1; sx++) {
          const s = (sy * src.width + sx) * 4;
          const sa = src.data[s + 3]!;
          r += src.data[s]! * sa;
          g += src.data[s + 1]! * sa;
          b += src.data[s + 2]! * sa;
          a += sa;
          n++;
        }
      }
      const d = (y * width + x) * 4;
      if (a === 0) {
        out[d] = 0; out[d + 1] = 0; out[d + 2] = 0; out[d + 3] = 0;
      } else {
        out[d] = Math.round(r / a);
        out[d + 1] = Math.round(g / a);
        out[d + 2] = Math.round(b / a);
        out[d + 3] = Math.round(a / n);
      }
    }
  }
  return { width, height, data: out };
}

/**
 * Snap every pixel to the nearest palette entry.
 *
 * Distance is weighted for perceived brightness (green dominates human
 * luminance response), which beats plain RGB euclidean for picking the
 * "closest looking" colour.
 */
export function quantize(
  img: Raster,
  palette: Rgba[],
  opts: { alphaThreshold?: number } = {},
): { raster: Raster; changed: number } {
  if (palette.length === 0) throw new Error('Palette is empty.');
  const threshold = opts.alphaThreshold ?? 128;

  const opaque = palette.filter((p) => p.a !== 0);
  if (opaque.length === 0) throw new Error('Palette contains no opaque colours.');

  const out = new Uint8Array(img.data.length);
  const cache = new Map<number, Rgba>();
  let changed = 0;

  for (let i = 0; i < img.data.length; i += 4) {
    const r = img.data[i]!, g = img.data[i + 1]!, b = img.data[i + 2]!, a = img.data[i + 3]!;

    if (a < threshold) {
      out[i] = 0; out[i + 1] = 0; out[i + 2] = 0; out[i + 3] = 0;
      if (a !== 0) changed++;
      continue;
    }

    const key = (r << 16) | (g << 8) | b;
    let best = cache.get(key);
    if (!best) {
      let bestDist = Infinity;
      for (const p of opaque) {
        const dr = r - p.r, dg = g - p.g, db = b - p.b;
        const d = dr * dr * 0.3 + dg * dg * 0.59 + db * db * 0.11;
        if (d < bestDist) { bestDist = d; best = p; }
      }
      cache.set(key, best!);
    }
    out[i] = best!.r; out[i + 1] = best!.g; out[i + 2] = best!.b; out[i + 3] = 255;
    if (best!.r !== r || best!.g !== g || best!.b !== b || a !== 255) changed++;
  }

  return { raster: { width: img.width, height: img.height, data: out }, changed };
}

export function countColors(img: Raster): number {
  const seen = new Set<number>();
  for (let i = 0; i < img.data.length; i += 4) {
    seen.add(
      (img.data[i]! << 24) | (img.data[i + 1]! << 16) | (img.data[i + 2]! << 8) | img.data[i + 3]!,
    );
  }
  return seen.size;
}
