// LibreSprite | Copyright (C) 2024-2026 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "tests/test.h"

#include "app/context.h"
#include "app/document.h"
#include "app/file/file.h"
#include "app/file/svg_dialect.h"
#include "app/ini_file.h"
#include "base/fs.h"
#include "base/path.h"
#include "doc/doc.h"
#include "tinyxml2.h"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

using namespace app;

namespace {

const doc::color_t kTransparent = doc::rgba(0, 0, 0, 0);
constexpr const char* kSquare = "M0 0h2v2h-2z";

std::string svgDoc(const std::string& paths, int w = 8, int h = 8)
{
  return "<svg viewBox=\"0 0 " + std::to_string(w) + " " + std::to_string(h)
    + "\">" + paths + "</svg>";
}

std::string svgPath(const std::string& fill, const std::string& d,
                    const std::string& attributes = "")
{
  return "<path fill=\"" + fill + "\" d=\"" + d + "\" " + attributes + "/>";
}

class SvgDialectTest : public ::testing::Test {
protected:
  SvgDialectRaster raster;
  std::string filename;

  void SetUp() override { push_config_state(); }

  void TearDown() override {
    if (!filename.empty())
      std::remove(filename.c_str());
    pop_config_state();
  }

  ::testing::AssertionResult parse(std::string_view svg) {
    std::string error;
    if (SvgDialect::parse(svg, raster, error))
      return ::testing::AssertionSuccess();
    return ::testing::AssertionFailure() << error;
  }
};

TEST_F(SvgDialectTest, NonzeroWinding)
{
  const struct {
    const char* name;
    const char* path;
    const char* pixels;
  } cases[] = {
    { "island in a hole", "M0 0h8v8h-8zM1 1v6h6v-6zM3 3h2v2h-2z",
      "########" "#......#" "#......#" "#..##..#"
      "#..##..#" "#......#" "#......#" "########" },
    { "nested donuts", "M0 0h8v8h-8zM1 1v6h6v-6zM2 2h4v4h-4zM3 3v2h2v-2zM4 4h1v1h-1z",
      "########" "#......#" "#.####.#" "#.#..#.#"
      "#.#.##.#" "#.####.#" "#......#" "########" },
  };
  const auto red = doc::rgba(255, 0, 0, 255);
  for (const auto& test : cases) {
    SCOPED_TRACE(test.name);
    ASSERT_TRUE(parse(svgDoc(svgPath("#f00", test.path))));
    ASSERT_EQ(64u, raster.pixels.size());
    for (size_t i = 0; i < raster.pixels.size(); ++i)
      EXPECT_EQ(test.pixels[i] == '#' ? red : kTransparent, raster.pixels[i])
        << "pixel " << i % 8 << "," << i / 8;
    EXPECT_TRUE(raster.hasAlpha);
  }
}

TEST_F(SvgDialectTest, MultipleColors)
{
  ASSERT_TRUE(parse(svgDoc(
    svgPath("#f00", kSquare) + svgPath("#0f0", "M3 0h2v2h-2z")
    + svgPath("#00f", "M0 3h2v2h-2z"), 6, 6)));

  EXPECT_EQ(doc::rgba(255, 0, 0, 255), raster.at(1, 1));
  EXPECT_EQ(doc::rgba(0, 255, 0, 255), raster.at(4, 1));
  EXPECT_EQ(doc::rgba(0, 0, 255, 255), raster.at(1, 4));
  EXPECT_EQ(kTransparent, raster.at(2, 2));
}

TEST_F(SvgDialectTest, ColorAndOpacity)
{
  const struct {
    const char* fill;
    const char* attributes;
    doc::color_t expected;
  } cases[] = {
    { "#aBc", "", doc::rgba(170, 187, 204, 255) },
    { "#AaBbCc", "", doc::rgba(170, 187, 204, 255) },
    { "#102030", R"(fill-opacity="0.5")", doc::rgba(16, 32, 48, 128) },
    { "#405060", R"(fill-opacity="0.25")", doc::rgba(64, 80, 96, 64) },
  };
  for (const auto& test : cases) {
    SCOPED_TRACE(svgPath(test.fill, kSquare, test.attributes));
    ASSERT_TRUE(parse(svgDoc(svgPath(test.fill, kSquare, test.attributes), 2, 2)));
    EXPECT_EQ(std::vector<doc::color_t>(4, test.expected), raster.pixels);
    EXPECT_EQ(doc::rgba_geta(test.expected) < 255, raster.hasAlpha);
  }
}

TEST_F(SvgDialectTest, XmlSyntaxAndEntities)
{
  ASSERT_TRUE(parse(R"(<?xml version='1.0'?>
    <svg viewBox = '0 0 2 2 '>
      <path data-fill='#f00' data-d='invalid' data-note='a > b'
            d = 'M0&#32;0h2v2h-2z' fill-opacity = '0.5' fill = '&#35;0f0'></path>
    </svg>)"));
  EXPECT_EQ(2, raster.width);
  EXPECT_EQ(2, raster.height);
  EXPECT_EQ(std::vector<doc::color_t>(4, doc::rgba(0, 255, 0, 128)), raster.pixels);
}

TEST_F(SvgDialectTest, IgnoresCommentsAndText)
{
  ASSERT_TRUE(parse("<!-- <svg viewBox=\"0 0 99 99\"> -->" + svgDoc(
    "<!-- " + svgPath("#f00", kSquare) + " -->"
    + svgPath("#0f0", kSquare)
    + "<desc><![CDATA[" + svgPath("#00f", kSquare) + "]]></desc>"
    + R"(<pathology fill="#fff" d="M0 0h2v2h-2z"/>)", 2, 2)));
  EXPECT_EQ(2, raster.width);
  EXPECT_EQ(2, raster.height);
  EXPECT_EQ(std::vector<doc::color_t>(4, doc::rgba(0, 255, 0, 255)), raster.pixels);
}

TEST_F(SvgDialectTest, InvalidInput)
{
  const std::string square = svgPath("#fff", kSquare);
  const std::pair<const char*, std::string> cases[] = {
    { "not XML", "just some text" },
    { "unclosed root", "<svg viewBox=\"0 0 2 2\">" + square },
    { "mismatched tags", R"(<svg viewBox="0 0 2 2"><path fill="#fff" d="M0 0h2v2h-2z"></svg>)" },
    { "duplicate attribute", svgDoc(svgPath("#fff", kSquare, R"(fill="#f00")")) },
    { "multiple roots", svgDoc(square) + "<svg/>" },
    { "wrong root", "<other>" + svgDoc(square) + "</other>" },
    { "partial tag name", R"(<svgish viewBox="0 0 2 2">)" + square + "</svgish>" },
    { "missing viewBox", "<svg>" + square + "</svg>" },
    { "partial viewBox name", R"(<svg data-viewBox="0 0 2 2">)" + square + "</svg>" },
    { "viewBox on path", "<svg>" + svgPath("#fff", kSquare, R"(viewBox="0 0 2 2")") + "</svg>" },
    { "viewBox trailing garbage", R"(<svg viewBox="0 0 2 2 garbage">)" + square + "</svg>" },
    { "no paths", svgDoc("") },
    { "bad color", svgDoc(svgPath("#gg0000", kSquare)) },
    { "partial fill name", svgDoc(R"(<path data-fill="#fff" d="M0 0h2v2h-2z"/>)") },
    { "partial data name", svgDoc(R"(<path fill="#fff" data-d="M0 0h2v2h-2z"/>)") },
    { "unknown command", svgDoc(svgPath("#fff", "M0 0x2v2h-2z")) },
    { "coordinate overflow", svgDoc(svgPath("#fff", "M0 0h99999999999999v2h-2z")) },
    { "oversized image", svgDoc(square, 65536, 65536) },
  };
  for (const auto& [name, svg] : cases) {
    SCOPED_TRACE(name);
    std::string error;
    EXPECT_FALSE(SvgDialect::parse(svg, raster, error));
    EXPECT_FALSE(error.empty());
  }
  for (const char* opacity : { "", "-0.1", "1.1", "nan", "inf", "0.5garbage" }) {
    SCOPED_TRACE(opacity);
    EXPECT_FALSE(parse(svgDoc(svgPath("#fff", kSquare,
      std::string("fill-opacity=\"") + opacity + "\""))));
  }
}

} // namespace

// Ensure the SVG format's static registration is linked into the test binary.
extern "C" void svg_format_anchor();

TEST_F(SvgDialectTest, SaveRoundTrip)
{
  svg_format_anchor();
  set_config_int("SVG", "Scale", 3);
  filename = base::join_path(base::get_temp_path(), "svg_dialect_tests.svg");
  app::Context context;
  auto* document = context.documents().add(16, 16, doc::ColorMode::RGB, 256);
  document->setFilename(filename);
  Image* image = document->sprite()->folder()->getFirstLayer()->cel(frame_t(0))->image();

  // Include every alpha byte, with the transparent pixel left uncovered.
  std::vector<doc::color_t> expected;
  for (int alpha = 0; alpha < 256; ++alpha) {
    const auto color = alpha ? doc::rgba(10, 20, 30, alpha) : kTransparent;
    put_pixel_fast<RgbTraits>(image, alpha % 16, alpha / 16, color);
    expected.push_back(color);
  }
  ASSERT_EQ(0, save_document(&context, document));
  document->close();
  delete document;

  std::ifstream input(filename, std::ios::binary);
  ASSERT_TRUE(input);
  const std::string svg{ std::istreambuf_iterator<char>(input), {} };
  tinyxml2::XMLDocument xml;
  ASSERT_EQ(tinyxml2::XML_SUCCESS, xml.Parse(svg.data(), svg.size())) << xml.ErrorStr();
  const auto* root = xml.FirstChildElement("svg");
  ASSERT_NE(nullptr, root);
  EXPECT_STREQ("http://www.w3.org/2000/svg", root->Attribute("xmlns"));
  EXPECT_STREQ("0 0 16 16", root->Attribute("viewBox"));
  EXPECT_STREQ("crispEdges", root->Attribute("shape-rendering"));
  EXPECT_EQ(48, root->IntAttribute("width"));
  EXPECT_EQ(48, root->IntAttribute("height"));
  for (const auto* path = root->FirstChildElement("path"); path;
       path = path->NextSiblingElement("path"))
    EXPECT_STREQ("nonzero", path->Attribute("fill-rule"));

  ASSERT_TRUE(parse(svg));
  EXPECT_EQ(16, raster.width);
  EXPECT_EQ(16, raster.height);
  EXPECT_EQ(expected, raster.pixels);
  EXPECT_TRUE(raster.hasAlpha);
}
