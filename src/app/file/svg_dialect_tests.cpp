// LibreSprite | Copyright (C) 2024-2026 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "tests/test.h"

#include "app/app.h"
#include "app/context.h"
#include "app/document.h"
#include "app/file/file.h"
#include "app/file/svg_dialect.h"
#include "app/ini_file.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "base/path.h"
#include "doc/doc.h"

#include <cstdio>
#include <string>

using namespace app;

static const doc::color_t kTransparent = doc::rgba(0, 0, 0, 0);

static std::string svgDoc(const std::string& paths, int w = 8, int h = 8)
{
  return
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 "
    + std::to_string(w) + " " + std::to_string(h)
    + "\" width=\"" + std::to_string(w) + "\" height=\"" + std::to_string(h)
    + "\" shape-rendering=\"crispEdges\">\n"
    + paths
    + "</svg>\n";
}

static bool parseSvg(const std::string& svgText, SvgDialectRaster& raster,
                     std::string& error)
{
  return SvgDialect::parse(svgText, raster, error);
}

// Same-color island inside the hole stays solid (nonzero winding).
TEST(SvgDialect, RingIslandInHole)
{
  SvgDialectRaster raster;
  std::string error;
  ASSERT_TRUE(parseSvg(svgDoc(
    "  <path fill=\"#f00\" fill-rule=\"nonzero\" "
    "d=\"M0 0h8v8h-8zM1 1v6h6v-6zM3 3h2v2h-2z\"/>\n"),
    raster, error)) << error;

  ASSERT_EQ(8, raster.width);
  ASSERT_EQ(8, raster.height);
  EXPECT_EQ(doc::rgba(255, 0, 0, 255), raster.at(0, 0));
  EXPECT_EQ(doc::rgba(255, 0, 0, 255), raster.at(7, 7));
  EXPECT_EQ(kTransparent, raster.at(1, 1));   // hole stays empty
  EXPECT_EQ(kTransparent, raster.at(6, 6));
  EXPECT_EQ(kTransparent, raster.at(2, 3));
  EXPECT_EQ(doc::rgba(255, 0, 0, 255), raster.at(3, 3)); // island stays solid
  EXPECT_EQ(doc::rgba(255, 0, 0, 255), raster.at(4, 4));
  EXPECT_TRUE(raster.hasAlpha); // the hole is transparent
}

TEST(SvgDialect, NestedDonutsSameColor)
{
  SvgDialectRaster raster;
  std::string error;
  ASSERT_TRUE(parseSvg(svgDoc(
    "  <path fill=\"#0f0\" fill-rule=\"nonzero\" "
    "d=\"M0 0h8v8h-8zM1 1v6h6v-6zM2 2h4v4h-4zM3 3v2h2v-2zM4 4h1v1h-1z\"/>\n"),
    raster, error)) << error;

  EXPECT_EQ(doc::rgba(0, 255, 0, 255), raster.at(0, 0)); // outer donut solid
  EXPECT_EQ(kTransparent, raster.at(1, 1));              // outer hole empty
  EXPECT_EQ(doc::rgba(0, 255, 0, 255), raster.at(2, 2)); // inner donut solid
  EXPECT_EQ(kTransparent, raster.at(3, 3));              // inner hole empty
  EXPECT_EQ(doc::rgba(0, 255, 0, 255), raster.at(4, 4)); // innermost dot solid
  EXPECT_EQ(doc::rgba(0, 255, 0, 255), raster.at(5, 5)); // inner donut solid
  EXPECT_EQ(kTransparent, raster.at(6, 6));
  EXPECT_EQ(doc::rgba(0, 255, 0, 255), raster.at(7, 7));
}

TEST(SvgDialect, MultipleDisjointColors)
{
  SvgDialectRaster raster;
  std::string error;
  ASSERT_TRUE(parseSvg(svgDoc(
    "  <path fill=\"#f00\" fill-rule=\"nonzero\" d=\"M0 0h2v2h-2z\"/>\n"
    "  <path fill=\"#0f0\" fill-rule=\"nonzero\" d=\"M3 0h2v2h-2z\"/>\n"
    "  <path fill=\"#00f\" fill-rule=\"nonzero\" d=\"M0 3h2v2h-2z\"/>\n",
    6, 6), raster, error)) << error;

  EXPECT_EQ(doc::rgba(255, 0, 0, 255), raster.at(1, 1));
  EXPECT_EQ(doc::rgba(0, 255, 0, 255), raster.at(4, 1));
  EXPECT_EQ(doc::rgba(0, 0, 255, 255), raster.at(1, 4));
  EXPECT_EQ(kTransparent, raster.at(2, 2));
  EXPECT_EQ(kTransparent, raster.at(2, 4));
  EXPECT_EQ(kTransparent, raster.at(4, 4));
}

TEST(SvgDialect, FillOpacityRoundsToAlpha)
{
  SvgDialectRaster raster;
  std::string error;
  ASSERT_TRUE(parseSvg(svgDoc(
    "  <path fill=\"#102030\" fill-opacity=\"0.5\" fill-rule=\"nonzero\" "
    "d=\"M0 0h2v2h-2z\"/>\n"
    "  <path fill=\"#405060\" fill-opacity=\"0.25\" fill-rule=\"nonzero\" "
    "d=\"M3 0h2v2h-2z\"/>\n"),
    raster, error)) << error;

  EXPECT_EQ(doc::rgba(0x10, 0x20, 0x30, 128), raster.at(1, 1));
  EXPECT_EQ(doc::rgba(0x40, 0x50, 0x60, 64), raster.at(4, 1));
  EXPECT_TRUE(raster.hasAlpha);
}

TEST(SvgDialect, HexColorShortAndLongForm)
{
  SvgDialectRaster raster;
  std::string error;
  ASSERT_TRUE(parseSvg(svgDoc(
    "  <path fill=\"#abc\" fill-rule=\"nonzero\" d=\"M0 0h2v2h-2z\"/>\n"
    "  <path fill=\"#aabbcc\" fill-rule=\"nonzero\" d=\"M3 0h2v2h-2z\"/>\n"),
    raster, error)) << error;

  EXPECT_EQ(doc::rgba(0xaa, 0xbb, 0xcc, 255), raster.at(1, 1));
  EXPECT_EQ(doc::rgba(0xaa, 0xbb, 0xcc, 255), raster.at(4, 1));
}

TEST(SvgDialect, GarbageInputFails)
{
  SvgDialectRaster raster;
  std::string error;

  // Truncated <path> tag (no '>' and no closing tag at all).
  error.clear();
  EXPECT_FALSE(parseSvg(
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 2 2\" "
    "width=\"2\" height=\"2\">\n"
    "  <path fill=\"#fff\" d=\"M0 0h2v2h-2z\"",
    raster, error));
  EXPECT_FALSE(error.empty());

  // Bad hex color.
  error.clear();
  EXPECT_FALSE(parseSvg(svgDoc(
    "  <path fill=\"#gg0000\" d=\"M0 0h2v2h-2z\"/>\n"), raster, error));
  EXPECT_FALSE(error.empty());

  // Bad path data (unknown command).
  error.clear();
  EXPECT_FALSE(parseSvg(svgDoc(
    "  <path fill=\"#fff\" d=\"M0 0 x2v2h-2z\"/>\n"), raster, error));
  EXPECT_FALSE(error.empty());

  // Out-of-int-range coordinate.
  error.clear();
  EXPECT_FALSE(parseSvg(svgDoc(
    "  <path fill=\"#fff\" d=\"M0 0h99999999999999v2h-2z\"/>\n"),
    raster, error));
  EXPECT_FALSE(error.empty());

  // Missing viewBox.
  error.clear();
  EXPECT_FALSE(parseSvg(
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"2\" height=\"2\">\n"
    "  <path fill=\"#fff\" d=\"M0 0h2v2h-2z\"/>\n"
    "</svg>\n", raster, error));
  EXPECT_FALSE(error.empty());

  // No <path> elements.
  error.clear();
  EXPECT_FALSE(parseSvg(svgDoc(""), raster, error));
  EXPECT_FALSE(error.empty());

  // Not an SVG file at all.
  error.clear();
  EXPECT_FALSE(parseSvg("just some text", raster, error));
  EXPECT_FALSE(error.empty());
}

// Links svg_format into the test binary so SvgFormat registers.
extern "C" void svg_format_anchor();

// End-to-end: save a sprite as SVG, then parse it back.
TEST(SvgDialect, SaveRoundTrip)
{
  svg_format_anchor();

  // Saver reads the persisted scale; needs config initialized.
  app::ConfigModule config;

  std::string fn = base::join_path(base::get_temp_path(),
                                   "svg_dialect_tests.svg");
  app::Context ctx;

  {
    doc::Document* doc = ctx.documents().add(8, 8, doc::ColorMode::RGB, 256);
    doc->setFilename(fn);

    Layer* layer = doc->sprite()->folder()->getFirstLayer();
    ASSERT_TRUE(layer != nullptr);
    Image* image = layer->cel(frame_t(0))->image();

    // Red ring with a red island in its hole, plus one
    // semi-transparent pixel for the fill-opacity round trip.
    const doc::color_t red = doc::rgba(255, 0, 0, 255);
    for (int y = 0; y < 8; ++y) {
      for (int x = 0; x < 8; ++x) {
        doc::color_t c = kTransparent;
        if (x == 0 || x == 7 || y == 0 || y == 7 ||
            (x >= 3 && x < 5 && y >= 3 && y < 5))
          c = red;
        else if (x == 1 && y == 1)
          c = doc::rgba(10, 20, 30, 128);
        put_pixel_fast<RgbTraits>(image, x, y, c);
      }
    }

    EXPECT_EQ(0, save_document(&ctx, doc));
    doc->close();
    delete doc;
  }

  // Read the written file back.
  std::string data;
  {
    base::FileHandle handle(base::open_file(fn, "rb"));
    ASSERT_TRUE(handle.get() != nullptr);
    if (fseek(handle.get(), 0, SEEK_END) != 0)
      FAIL();
    long len = ftell(handle.get());
    ASSERT_GT(len, 0);
    if (fseek(handle.get(), 0, SEEK_SET) != 0)
      FAIL();
    data.resize(size_t(len));
    ASSERT_EQ(size_t(len), fread(&data[0], 1, size_t(len), handle.get()));
  }

  // Saver must emit an explicit nonzero fill rule.
  ASSERT_TRUE(data.find("fill-rule=\"nonzero\"") != std::string::npos)
    << data;

  // ... and it must parse back through the dialect reader.
  SvgDialectRaster raster;
  std::string error;
  ASSERT_TRUE(parseSvg(data, raster, error)) << error;
  ASSERT_EQ(8, raster.width);
  ASSERT_EQ(8, raster.height);

  // The fill-opacity text must parse back to the same alpha byte.
  EXPECT_TRUE(data.find("fill-opacity=\"") != std::string::npos) << data;

  const doc::color_t red = doc::rgba(255, 0, 0, 255);
  for (int y = 0; y < 8; ++y) {
    for (int x = 0; x < 8; ++x) {
      doc::color_t expected = kTransparent;
      if (x == 0 || x == 7 || y == 0 || y == 7 ||
          (x >= 3 && x < 5 && y >= 3 && y < 5))
        expected = red;
      else if (x == 1 && y == 1)
        expected = doc::rgba(10, 20, 30, 128);
      ASSERT_EQ(expected, raster.at(x, y))
        << "pixel " << x << "," << y;
    }
  }

  std::remove(fn.c_str());
}
