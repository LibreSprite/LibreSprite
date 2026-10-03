import { deflateSync, inflateSync } from 'node:zlib';

/**
 * Minimal PNG codec, no dependencies.
 *
 * Pixels are handled on the host rather than in the container because reading a
 * 256x256 image one getPixel() at a time through Duktape would take ~20s. The
 * container stays responsible for what only it can do: the .ase format.
 *
 * Decode supports 8-bit greyscale/RGB/palette/greyscale+alpha/RGBA, non-interlaced.
 * Encode always writes 8-bit RGBA.
 */

export interface Raster {
  width: number;
  height: number;
  /** RGBA, 4 bytes per pixel, row-major. */
  data: Uint8Array;
}

const SIGNATURE = Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]);

const CRC_TABLE = (() => {
  const t = new Int32Array(256);
  for (let n = 0; n < 256; n++) {
    let c = n;
    for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
    t[n] = c;
  }
  return t;
})();

function crc32(buf: Buffer): number {
  let c = 0xffffffff;
  for (let i = 0; i < buf.length; i++) c = CRC_TABLE[(c ^ buf[i]!) & 0xff]! ^ (c >>> 8);
  return (c ^ 0xffffffff) >>> 0;
}

function chunk(type: string, body: Buffer): Buffer {
  const len = Buffer.alloc(4);
  len.writeUInt32BE(body.length, 0);
  const typeAndBody = Buffer.concat([Buffer.from(type, 'ascii'), body]);
  const crc = Buffer.alloc(4);
  crc.writeUInt32BE(crc32(typeAndBody), 0);
  return Buffer.concat([len, typeAndBody, crc]);
}

const paeth = (a: number, b: number, c: number): number => {
  const p = a + b - c;
  const pa = Math.abs(p - a);
  const pb = Math.abs(p - b);
  const pc = Math.abs(p - c);
  return pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
};

export function decodePng(buffer: Buffer): Raster {
  if (buffer.length < 8 || !buffer.subarray(0, 8).equals(SIGNATURE)) {
    throw new Error('Not a PNG file (bad signature).');
  }

  let width = 0;
  let height = 0;
  let depth = 0;
  let colorType = 0;
  let interlace = 0;
  let palette: Buffer | undefined;
  let transparency: Buffer | undefined;
  const idat: Buffer[] = [];

  let off = 8;
  while (off + 8 <= buffer.length) {
    const len = buffer.readUInt32BE(off);
    const type = buffer.toString('ascii', off + 4, off + 8);
    const body = buffer.subarray(off + 8, off + 8 + len);
    off += 12 + len;

    if (type === 'IHDR') {
      width = body.readUInt32BE(0);
      height = body.readUInt32BE(4);
      depth = body[8]!;
      colorType = body[9]!;
      interlace = body[12]!;
    } else if (type === 'PLTE') palette = Buffer.from(body);
    else if (type === 'tRNS') transparency = Buffer.from(body);
    else if (type === 'IDAT') idat.push(Buffer.from(body));
    else if (type === 'IEND') break;
  }

  if (depth !== 8) throw new Error(`Unsupported PNG bit depth ${depth}; only 8-bit is handled.`);
  if (interlace !== 0) throw new Error('Interlaced PNGs are not supported.');
  if (idat.length === 0) throw new Error('PNG has no image data.');

  const channels = { 0: 1, 2: 3, 3: 1, 4: 2, 6: 4 }[colorType];
  if (!channels) throw new Error(`Unsupported PNG colour type ${colorType}.`);

  const raw = inflateSync(Buffer.concat(idat));
  const stride = width * channels;
  const out = new Uint8Array(width * height * 4);
  const line = new Uint8Array(stride);
  const prev = new Uint8Array(stride);

  let p = 0;
  for (let y = 0; y < height; y++) {
    const filter = raw[p++]!;
    for (let i = 0; i < stride; i++) {
      const x = raw[p + i]!;
      const a = i >= channels ? line[i - channels]! : 0;
      const b = prev[i]!;
      const c = i >= channels ? prev[i - channels]! : 0;
      let v: number;
      switch (filter) {
        case 0: v = x; break;
        case 1: v = x + a; break;
        case 2: v = x + b; break;
        case 3: v = x + ((a + b) >> 1); break;
        case 4: v = x + paeth(a, b, c); break;
        default: throw new Error(`Unknown PNG filter ${filter} on row ${y}.`);
      }
      line[i] = v & 0xff;
    }
    p += stride;

    for (let x = 0; x < width; x++) {
      const s = x * channels;
      const d = (y * width + x) * 4;
      if (colorType === 0) {
        const g = line[s]!;
        out[d] = g; out[d + 1] = g; out[d + 2] = g; out[d + 3] = 255;
      } else if (colorType === 2) {
        out[d] = line[s]!; out[d + 1] = line[s + 1]!; out[d + 2] = line[s + 2]!; out[d + 3] = 255;
      } else if (colorType === 3) {
        const idx = line[s]!;
        if (!palette) throw new Error('Indexed PNG without a PLTE chunk.');
        out[d] = palette[idx * 3]!; out[d + 1] = palette[idx * 3 + 1]!; out[d + 2] = palette[idx * 3 + 2]!;
        out[d + 3] = transparency && idx < transparency.length ? transparency[idx]! : 255;
      } else if (colorType === 4) {
        const g = line[s]!;
        out[d] = g; out[d + 1] = g; out[d + 2] = g; out[d + 3] = line[s + 1]!;
      } else {
        out[d] = line[s]!; out[d + 1] = line[s + 1]!; out[d + 2] = line[s + 2]!; out[d + 3] = line[s + 3]!;
      }
    }
    prev.set(line);
  }

  return { width, height, data: out };
}

export function encodePng(img: Raster): Buffer {
  const stride = img.width * 4;
  const raw = Buffer.alloc((stride + 1) * img.height);
  for (let y = 0; y < img.height; y++) {
    raw[y * (stride + 1)] = 0; // filter: none
    Buffer.from(img.data.buffer, img.data.byteOffset + y * stride, stride).copy(
      raw,
      y * (stride + 1) + 1,
    );
  }

  const ihdr = Buffer.alloc(13);
  ihdr.writeUInt32BE(img.width, 0);
  ihdr.writeUInt32BE(img.height, 4);
  ihdr[8] = 8; // bit depth
  ihdr[9] = 6; // RGBA
  ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;

  return Buffer.concat([
    SIGNATURE,
    chunk('IHDR', ihdr),
    chunk('IDAT', deflateSync(raw, { level: 9 })),
    chunk('IEND', Buffer.alloc(0)),
  ]);
}

export function blankRaster(width: number, height: number, rgba: [number, number, number, number]): Raster {
  const data = new Uint8Array(width * height * 4);
  for (let i = 0; i < data.length; i += 4) {
    data[i] = rgba[0]; data[i + 1] = rgba[1]; data[i + 2] = rgba[2]; data[i + 3] = rgba[3];
  }
  return { width, height, data };
}
