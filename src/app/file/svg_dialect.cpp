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
#include "tinyxml2.h"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <locale>
#include <sstream>
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
  const size_t first = s.find_first_not_of(" \t\n\r,");
  s.remove_prefix(first == std::string_view::npos ? s.size() : first);
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
static bool parseViewBox(std::string_view v, int& width, int& height)
{
  int x0 = 0, y0 = 0;
  if (!parseInt(v, x0) || !parseInt(v, y0) ||
      !parseInt(v, width) || !parseInt(v, height))
    return false;
  skipSeparators(v);
  return v.empty() && x0 == 0 && y0 == 0 && width > 0 && height > 0;
}

// "#rgb" (each digit doubled) or "#rrggbb".
static bool parseHexColor(std::string_view s, doc::color_t& out)
{
  if ((s.size() != 4 && s.size() != 7) || s.front() != '#')
    return false;

  unsigned rgb = 0;
  const auto result = std::from_chars(s.data() + 1, s.data() + s.size(), rgb, 16);
  if (result.ec != std::errc() || result.ptr != s.data() + s.size())
    return false;

  if (s.size() == 4)
    out = rgba((rgb >> 8) * 17, ((rgb >> 4) & 15) * 17, (rgb & 15) * 17, 255);
  else
    out = rgba(rgb >> 16, (rgb >> 8) & 255, rgb & 255, 255);
  return true;
}

static bool parseFillOpacity(std::string_view s, uint8_t& alpha)
{
  std::istringstream input{std::string(s)};
  input.imbue(std::locale::classic());
  double opacity = 0;
  input >> std::noskipws >> opacity;
  if (!input || !input.eof() || !(opacity >= 0 && opacity <= 1))
    return false;

  alpha = uint8_t(std::lround(255 * opacity));
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

// Evaluate SVG's nonzero fill rule with exact integer arithmetic.
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
  tinyxml2::XMLDocument xml;
  if (xml.Parse(svg.data(), svg.size()) != tinyxml2::XML_SUCCESS) {
    error = std::string("SVG: invalid XML: ") + xml.ErrorStr() + "\n";
    return false;
  }

  const auto* root = xml.RootElement();
  if (!root || std::string_view(root->Name()) != "svg" ||
      root->NextSiblingElement()) {
    error = "SVG: expected a single <svg> root element\n";
    return false;
  }

  int width = 0, height = 0;
  const char* viewBox = root->Attribute("viewBox");
  if (!viewBox || !parseViewBox(viewBox, width, height)) {
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
  for (const auto* path = root->FirstChildElement("path");
       path; path = path->NextSiblingElement("path")) {
    PathGroup group;
    const char* fill = path->Attribute("fill");
    if (!fill || !parseHexColor(fill, group.color)) {
      error = "SVG: invalid or missing fill color in <path>\n";
      return false;
    }
    if (const char* opacity = path->Attribute("fill-opacity")) {
      uint8_t alpha;
      if (!parseFillOpacity(opacity, alpha)) {
        error = "SVG: invalid fill-opacity in <path>\n";
        return false;
      }
      group.color = rgba(rgba_getr(group.color), rgba_getg(group.color),
                         rgba_getb(group.color), alpha);
    }
    const char* d = path->Attribute("d");
    if (!d || !parseSvgPathData(d, group.polys)) {
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
