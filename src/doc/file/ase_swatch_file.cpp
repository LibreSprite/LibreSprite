// Aseprite Document Library
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/color.h"
#include "doc/palette.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

namespace doc {
namespace file {

namespace {

// Adobe Swatch Exchange is a big-endian format.
uint16_t read_u16_be(FILE* f) {
  int hi = std::fgetc(f);
  int lo = std::fgetc(f);
  return (uint16_t)(((hi & 0xff) << 8) | (lo & 0xff));
}

uint32_t read_u32_be(FILE* f) {
  uint32_t b0 = (uint32_t)std::fgetc(f);
  uint32_t b1 = (uint32_t)std::fgetc(f);
  uint32_t b2 = (uint32_t)std::fgetc(f);
  uint32_t b3 = (uint32_t)std::fgetc(f);
  return ((b0 & 0xff) << 24) | ((b1 & 0xff) << 16) | ((b2 & 0xff) << 8) | (b3 & 0xff);
}

float read_f32_be(FILE* f) {
  uint32_t u = read_u32_be(f);
  float v;
  std::memcpy(&v, &u, sizeof(v));
  return v;
}

uint8_t to_byte(float v) {
  long n = std::lround(v * 255.0f);
  if (n < 0) n = 0;
  if (n > 255) n = 255;
  return (uint8_t)n;
}

// CIELAB (D50) -> sRGB, for the rare .ase files that store Lab colors.
void lab_to_rgb(float L, float a, float b, uint8_t& R, uint8_t& G, uint8_t& B) {
  float fy = (L + 16.0f) / 116.0f;
  float fx = fy + a / 500.0f;
  float fz = fy - b / 200.0f;
  auto finv = [](float t) {
    float t3 = t * t * t;
    return (t3 > 0.008856f) ? t3 : (t - 16.0f / 116.0f) / 7.787f;
  };
  // D50 reference white.
  float X = 0.96422f * finv(fx);
  float Y = 1.00000f * finv(fy);
  float Z = 0.82521f * finv(fz);
  // XYZ (D50) -> linear sRGB (Bradford-adapted matrix).
  float r =  3.1338561f * X - 1.6168667f * Y - 0.4906146f * Z;
  float g = -0.9787684f * X + 1.9161415f * Y + 0.0334540f * Z;
  float bl =  0.0719453f * X - 0.2289914f * Y + 1.4052427f * Z;
  auto gamma = [](float c) {
    if (c <= 0.0f) return 0.0f;
    if (c >= 1.0f) return 1.0f;
    return (c <= 0.0031308f) ? 12.92f * c : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
  };
  R = to_byte(gamma(r));
  G = to_byte(gamma(g));
  B = to_byte(gamma(bl));
}

} // anonymous namespace

std::shared_ptr<Palette> load_ase_swatch_file(const char* filename) {
  FILE* f = std::fopen(filename, "rb");
  if (!f)
    return nullptr;

  char sig[4];
  if (std::fread(sig, 1, 4, f) != 4 || std::memcmp(sig, "ASEF", 4) != 0) {
    std::fclose(f);
    return nullptr;            // Not an Adobe Swatch Exchange file.
  }

  read_u16_be(f);              // version major
  read_u16_be(f);              // version minor
  uint32_t blockCount = read_u32_be(f);

  std::vector<color_t> colors;
  for (uint32_t i = 0; i < blockCount; ++i) {
    uint16_t type = read_u16_be(f);
    uint32_t length = read_u32_be(f);
    long blockStart = std::ftell(f);
    if (blockStart < 0 || std::feof(f))
      break;

    if (type == 0x0001) {      // Color entry
      uint16_t nameLen = read_u16_be(f);          // UTF-16 units incl. null
      std::fseek(f, (long)nameLen * 2, SEEK_CUR); // skip the swatch name

      char model[4] = {0};
      std::fread(model, 1, 4, f);
      uint8_t r = 0, g = 0, b = 0;
      if (std::memcmp(model, "RGB ", 4) == 0) {
        r = to_byte(read_f32_be(f));
        g = to_byte(read_f32_be(f));
        b = to_byte(read_f32_be(f));
      }
      else if (std::memcmp(model, "Gray", 4) == 0) {
        r = g = b = to_byte(read_f32_be(f));
      }
      else if (std::memcmp(model, "CMYK", 4) == 0) {
        float c = read_f32_be(f), m = read_f32_be(f);
        float y = read_f32_be(f), k = read_f32_be(f);
        r = to_byte((1.0f - c) * (1.0f - k));
        g = to_byte((1.0f - m) * (1.0f - k));
        b = to_byte((1.0f - y) * (1.0f - k));
      }
      else if (std::memcmp(model, "LAB ", 4) == 0) {
        float L = read_f32_be(f) * 100.0f;        // stored 0..1
        float A = read_f32_be(f);                 // stored -128..127
        float Bc = read_f32_be(f);
        lab_to_rgb(L, A, Bc, r, g, b);
      }
      else {
        // Unknown color model: skip this color but keep parsing.
        std::fseek(f, blockStart + (long)length, SEEK_SET);
        continue;
      }
      colors.push_back(rgba(r, g, b, 255));
    }

    // Always seek to the declared end of the block (handles groups, the
    // trailing color-type field, and any padding).
    std::fseek(f, blockStart + (long)length, SEEK_SET);
  }

  std::fclose(f);

  if (colors.empty())
    return nullptr;

  std::shared_ptr<Palette> pal = Palette::create((int)colors.size());
  for (int i = 0; i < (int)colors.size(); ++i)
    pal->setEntry(i, colors[i]);
  return pal;
}

} // namespace file
} // namespace doc
