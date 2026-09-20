// LibreSprite | Copyright (C) 2024-2026 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/file/svg_dialect.h"

#include "app/file/file.h"
#include "base/file_handle.h"
#include "doc/doc.h"
#include "gfx/point.h"
#include "gfx/rect.h"

#include <charconv>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace app {

namespace {

// One <path> per color, so cap paths to bound parse memory.
constexpr size_t kMaxPaths = 262144;

constexpr int64_t kMaxPixels = 64 * 1024 * 1024;
constexpr long kMaxFileBytes = 64 * 1024 * 1024;

// Coordinates are doubled: pixel centers are odd, polygon points
// even, so centers never lie on edges and rasterization is exact.
constexpr int kMaxCoord = 1 << 26;
constexpr int kMaxCoord2 = 2 * kMaxCoord;

// All subpaths of one <path> element, in fill order.
struct PathGroup {
  doc::color_t color = 0;
  std::vector<std::vector<gfx::Point>> polys;  // doubled coordinates
  gfx::Rect bounds;                            // union of all points
};

static void skipSeparators(std::string_view& s)
{
  size_t i = 0;
  while (i < s.size() &&
         (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r' || s[i] == ','))
    ++i;
  s.remove_prefix(i);
}

// from_chars rejects garbage and out-of-range values.
static bool parseInt(std::string_view& s, int& out)
{
  skipSeparators(s);
  if (s.empty())
    return false;
  auto res = std::from_chars(s.data(), s.data() + s.size(), out);
  if (res.ec != std::errc())
    return false;
  s.remove_prefix(size_t(res.ptr - s.data()));
  return true;
}

// Our writer always emits viewBox="0 0 W H".
static bool parseViewBox(std::string_view svg, int& width, int& height)
{
  size_t vb = svg.find("viewBox=\"");
  if (vb == std::string_view::npos)
    return false;
  vb += 9;
  size_t endQ = svg.find('"', vb);
  if (endQ == std::string_view::npos)
    return false;
  std::string_view v = svg.substr(vb, endQ - vb);
  int x0 = 0, y0 = 0;
  if (!parseInt(v, x0) || !parseInt(v, y0) ||
      !parseInt(v, width) || !parseInt(v, height))
    return false;
  return x0 == 0 && y0 == 0 && width > 0 && height > 0;
}

// "#rgb" (each digit doubled) or "#rrggbb".
static bool parseHexColor(std::string_view s, doc::color_t& out)
{
  auto nibble = [](char ch) -> int {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
  };
  if (s.size() != 4 && s.size() != 7)
    return false;
  if (s[0] != '#')
    return false;
  int v[6];
  for (int i = 0; i < 6; ++i) {
    // The short form indexes each nibble twice ("#abc" -> "aabbcc").
    int n = nibble(s[1 + (s.size() == 4 ? i / 2 : i)]);
    if (n < 0)
      return false;
    v[i] = n;
  }
  out = rgba(v[0] * 16 + v[1], v[2] * 16 + v[3], v[4] * 16 + v[5], 255);
  return true;
}

// Parses plain-decimal fill-opacity in fixed point for exact alpha.
static bool parseFillOpacity(std::string_view s, uint8_t& alpha)
{
  if (!s.empty() && s.front() == '-')
    return false;
  int whole = 0;
  if (!parseInt(s, whole))
    return false;

  int64_t num = whole;    // value * scale
  int64_t scale = 1;
  if (!s.empty() && s.front() == '.') {
    s.remove_prefix(1);
    while (!s.empty() && s.front() >= '0' && s.front() <= '9' &&
           scale <= 100000000) {
      num = num * 10 + (s.front() - '0');
      scale *= 10;
      s.remove_prefix(1);
    }
  }
  if (!s.empty() || whole < 0 || num > scale) // reject garbage and opacity > 1
    return false;

  alpha = uint8_t((255 * num + scale / 2) / scale); // round to nearest
  return true;
}

// Only the writer's grammar: absolute "M x y", relative "h"/"v", "z".
static bool parseSvgPathData(std::string_view d,
                             std::vector<std::vector<gfx::Point>>& polys)
{
  std::vector<gfx::Point> cur;
  bool have = false;
  auto flush = [&]() {
    if (have && cur.size() >= 3)
      polys.push_back(cur);
    cur.clear();
    have = false;
  };
  auto push = [&](int x, int y) -> bool {
    if (x < -kMaxCoord2 || x > kMaxCoord2 || y < -kMaxCoord2 || y > kMaxCoord2)
      return false;
    cur.push_back(gfx::Point(x, y));
    return true;
  };
  auto coord = [](std::string_view& s, int& v) -> bool {
    return parseInt(s, v) && v >= -kMaxCoord && v <= kMaxCoord;
  };

  while (true) {
    skipSeparators(d);
    if (d.empty())
      break;
    char cmd = d.front();
    d.remove_prefix(1);
    if (cmd == 'M') {
      int x, y;
      if (!coord(d, x) || !coord(d, y))
        return false;
      flush();
      have = push(2 * x, 2 * y);
    }
    else if (cmd == 'h' || cmd == 'v') {
      int dcv;
      if (!have || !coord(d, dcv))
        return false;
      int x = cur.back().x, y = cur.back().y;
      if (cmd == 'h') x += 2 * dcv;
      else            y += 2 * dcv;
      if (!push(x, y))
        return false;
    }
    else if (cmd == 'z') {
      flush();
    }
    else {
      return false;
    }
  }
  flush();
  return !polys.empty();
}

static std::string_view svgXmlAttr(std::string_view tag, const char* name)
{
  std::string key = std::string(name) + "=\"";
  size_t p = tag.find(key);
  if (p == std::string_view::npos)
    return std::string_view();
  p += key.size();
  size_t q = tag.find('"', p);
  if (q == std::string_view::npos)
    return std::string_view();
  return tag.substr(p, q - p);
}

// Nonzero winding in exact integer math. Holes and islands use
// opposite orientations, so even-odd would cancel them.
static bool windingNonzero(int px, int py,
                           const std::vector<std::vector<gfx::Point>>& polys)
{
  int winding = 0;
  for (const auto& poly : polys) {
    const size_t n = poly.size();
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
      const gfx::Point& p = poly[i];
      const gfx::Point& q = poly[j];
      int64_t cross = int64_t(q.x - p.x) * (py - p.y)
                    - int64_t(px - p.x) * (q.y - p.y);
      if (p.y <= py) {
        if (q.y > py && cross > 0)
          ++winding;
      }
      else if (q.y <= py && cross < 0) {
        --winding;
      }
    }
  }
  return winding != 0;
}

} // anonymous namespace

bool SvgDialect::parse(std::string_view svg,
                       SvgDialectRaster& raster,
                       std::string& error,
                       const ProgressFn& progress)
{
  if (svg.find("<svg") == std::string_view::npos) {
    error = "SVG: no <svg> element found\n";
    return false;
  }

  int width = 0, height = 0;
  if (!parseViewBox(svg, width, height)) {
    error = "SVG: missing or invalid viewBox\n";
    return false;
  }
  if ((int64_t)width * (int64_t)height > kMaxPixels) {
    char buf[128];
    std::snprintf(buf, sizeof(buf),
                  "SVG image dimensions too large (%dx%d)\n", width, height);
    error = buf;
    return false;
  }

  std::vector<PathGroup> groups;
  size_t pos = 0;
  while ((pos = svg.find("<path", pos)) != std::string_view::npos) {
    size_t end = svg.find('>', pos);
    if (end == std::string_view::npos) {
      error = "SVG: truncated <path> element\n";
      return false;
    }
    std::string_view tag = svg.substr(pos, end - pos);
    pos = end + 1;

    PathGroup group;
    if (!parseHexColor(svgXmlAttr(tag, "fill"), group.color)) {
      error = "SVG: invalid or missing fill color in <path>\n";
      return false;
    }
    if (std::string_view opacity = svgXmlAttr(tag, "fill-opacity"); !opacity.empty()) {
      uint8_t alpha;
      if (!parseFillOpacity(opacity, alpha)) {
        error = "SVG: invalid fill-opacity in <path>\n";
        return false;
      }
      group.color = rgba(rgba_getr(group.color), rgba_getg(group.color),
                         rgba_getb(group.color), alpha);
    }
    std::string_view d = svgXmlAttr(tag, "d");
    if (d.empty() || !parseSvgPathData(d, group.polys)) {
      error = "SVG: invalid path data\n";
      return false;
    }
    for (const auto& poly : group.polys)
      for (const gfx::Point& pt : poly)
        group.bounds |= gfx::Rect(pt.x, pt.y, 1, 1);

    groups.push_back(std::move(group));
    if (groups.size() > kMaxPaths) {
      error = "SVG: too many <path> elements\n";
      return false;
    }
  }

  if (groups.empty()) {
    error = "SVG: no drawable <path> elements found\n";
    return false;
  }

  raster.width = width;
  raster.height = height;
  raster.pixels.assign(size_t(width) * size_t(height), rgba(0, 0, 0, 0));
  raster.hasAlpha = false;

  // Later paths paint over earlier ones. Bounds cull is exact:
  // centers are odd, points even.
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const int px = 2 * x + 1;
      const int py = 2 * y + 1;
      doc::color_t c = rgba(0, 0, 0, 0);
      for (const PathGroup& group : groups) {
        if (group.bounds.contains(px, py) && windingNonzero(px, py, group.polys))
          c = group.color;
      }
      if (rgba_geta(c) < 255)
        raster.hasAlpha = true;
      raster.pixels[size_t(y) * size_t(width) + size_t(x)] = c;
    }
    if (progress && !progress(double(y + 1) / double(height)))
      return false;
  }
  return true;
}

bool SvgDialect::load(FileOp* fop)
{
  base::FileHandle handle;
  try {
    handle = base::open_file_with_exception(fop->filename(), "rb");
  }
  catch (const std::exception& e) {
    fop->setError("Error opening SVG file: %s\n", e.what());
    return false;
  }

  auto readFailed = [&]() {
    fop->setError("Error reading SVG file \"%s\"\n", fop->filename().c_str());
    return false;
  };
  if (fseek(handle.get(), 0, SEEK_END) != 0)
    return readFailed();
  long len = ftell(handle.get());
  if (len <= 0 || len > kMaxFileBytes)
    return readFailed();
  if (fseek(handle.get(), 0, SEEK_SET) != 0)
    return readFailed();

  std::string data(size_t(len), '\0');
  if (fread(&data[0], 1, size_t(len), handle.get()) != size_t(len))
    return readFailed();

  SvgDialectRaster raster;
  std::string error;
  auto progress = [fop](double fraction) -> bool {
    fop->setProgress(fraction);
    return !fop->isStop();
  };
  if (!parse(data, raster, error, progress)) {
    if (error.empty())
      error = "SVG loading cancelled\n";
    fop->setError("%s", error.c_str());
    return false;
  }

  Image* image = fop->sequenceImage(IMAGE_RGB, raster.width, raster.height);
  if (!image) {
    fop->setError("Error creating SVG image\n");
    return false;
  }

  for (int y = 0; y < raster.height; ++y) {
    for (int x = 0; x < raster.width; ++x)
      put_pixel(image, x, y, raster.at(x, y));
  }
  fop->sequenceSetHasAlpha(raster.hasAlpha);
  return true;
}

} // namespace app
