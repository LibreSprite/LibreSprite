export interface Rgba {
  r: number;
  g: number;
  b: number;
  a: number;
}

const HEX = /^#?([0-9a-f]{3,8})$/i;

/**
 * Accepts #rgb, #rgba, #rrggbb and #rrggbbaa. Alpha defaults to opaque.
 * Colours are never packed into an int on this side: doc::rgba puts alpha in
 * bit 31 (src/doc/color.h:48-51) and JS bitwise maths would sign-flip it, so
 * the components travel to the script and app.pixelColor.rgba does the packing.
 */
export function parseColor(input: string): Rgba {
  const m = HEX.exec(input.trim());
  if (!m || !m[1]) {
    throw new Error(`Invalid colour "${input}". Use #rgb, #rgba, #rrggbb or #rrggbbaa.`);
  }
  const hex = m[1];
  const expand = (s: string): number => parseInt(s.length === 1 ? s + s : s, 16);

  if (hex.length === 3 || hex.length === 4) {
    return {
      r: expand(hex[0]!),
      g: expand(hex[1]!),
      b: expand(hex[2]!),
      a: hex.length === 4 ? expand(hex[3]!) : 255,
    };
  }
  if (hex.length === 6 || hex.length === 8) {
    return {
      r: expand(hex.slice(0, 2)),
      g: expand(hex.slice(2, 4)),
      b: expand(hex.slice(4, 6)),
      a: hex.length === 8 ? expand(hex.slice(6, 8)) : 255,
    };
  }
  throw new Error(`Invalid colour "${input}". Expected 3, 4, 6 or 8 hex digits.`);
}

const pad2 = (n: number): string => n.toString(16).padStart(2, '0');

/** Canonical 8-digit lowercase rgba key, matching lsKey() in the ES5 prelude. */
export function colorKey(c: Rgba): string {
  return pad2(c.r) + pad2(c.g) + pad2(c.b) + pad2(c.a);
}

export function normalizeColorKey(input: string): string {
  return colorKey(parseColor(input));
}
