// LibreSprite | Copyright (C) 2026 LibreSprite-MCP contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/mcp/mcp_tools.h"
#include "app/mcp/mcp_ui_bridge.h"

#include "app/app.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/document.h"
#include "app/modules/palettes.h"
#include "app/script/app_scripting.h"
#include "app/ui_context.h"
#include "doc/cel.h"
#include "doc/color.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/site.h"
#include "doc/sprite.h"
#include "ui/manager.h"

#include <cctype>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>

namespace app {
namespace mcp {
namespace {

using nlohmann::json;

doc::color_t parseColor(const json& value, doc::PixelFormat format)
{
  int r = 0, g = 0, b = 0, a = 255;

  if (value.is_string()) {
    std::string s = value.get<std::string>();
    if (!s.empty() && s[0] == '#')
      s = s.substr(1);
    if (s.size() == 6 || s.size() == 8) {
      auto hex = [](char c) -> int {
        c = std::tolower(static_cast<unsigned char>(c));
        if (c >= '0' && c <= '9') return c - '0';
        return 10 + (c - 'a');
      };
      r = hex(s[0]) * 16 + hex(s[1]);
      g = hex(s[2]) * 16 + hex(s[3]);
      b = hex(s[4]) * 16 + hex(s[5]);
      if (s.size() == 8)
        a = hex(s[6]) * 16 + hex(s[7]);
    }
    else {
      throw std::runtime_error("Color string must be #RRGGBB or #RRGGBBAA");
    }
  }
  else if (value.is_object()) {
    r = value.value("r", 0);
    g = value.value("g", 0);
    b = value.value("b", 0);
    a = value.value("a", 255);
  }
  else if (value.is_number_integer()) {
    return static_cast<doc::color_t>(value.get<int>());
  }
  else {
    throw std::runtime_error("Unsupported color format");
  }

  switch (format) {
    case doc::IMAGE_RGB:
      return doc::rgba(r, g, b, a);
    case doc::IMAGE_GRAYSCALE:
      return doc::graya(doc::rgb_luma(r, g, b), a);
    case doc::IMAGE_INDEXED:
      return static_cast<doc::color_t>(r); // treat r as palette index when numeric object used
    default:
      return doc::rgba(r, g, b, a);
  }
}

doc::PixelFormat parseColorMode(const json& args)
{
  if (!args.contains("color_mode") && !args.contains("colorMode"))
    return doc::IMAGE_RGB;

  const json& v = args.contains("color_mode") ? args["color_mode"] : args["colorMode"];
  if (v.is_number_integer()) {
    int n = v.get<int>();
    if (n == doc::IMAGE_RGB || n == doc::IMAGE_GRAYSCALE || n == doc::IMAGE_INDEXED)
      return static_cast<doc::PixelFormat>(n);
  }
  if (v.is_string()) {
    std::string s = v.get<std::string>();
    for (char& c : s) c = std::tolower(static_cast<unsigned char>(c));
    if (s == "rgb" || s == "rgba") return doc::IMAGE_RGB;
    if (s == "grayscale" || s == "gray") return doc::IMAGE_GRAYSCALE;
    if (s == "indexed" || s == "index") return doc::IMAGE_INDEXED;
  }
  throw std::runtime_error("color_mode must be rgb, grayscale, or indexed");
}

const char* colorModeName(doc::PixelFormat pf)
{
  switch (pf) {
    case doc::IMAGE_RGB: return "rgb";
    case doc::IMAGE_GRAYSCALE: return "grayscale";
    case doc::IMAGE_INDEXED: return "indexed";
    default: return "unknown";
  }
}

app::Document* requireDocument()
{
  auto* ctx = UIContext::instance();
  if (!ctx)
    throw std::runtime_error("UI context is not available");
  auto* doc = ctx->activeDocument();
  if (!doc)
    throw std::runtime_error("No active document");
  return doc;
}

doc::Image* requireActiveImage(app::Document** outDoc = nullptr)
{
  auto* doc = requireDocument();
  if (outDoc) *outDoc = doc;
  doc::Site site = UIContext::instance()->activeSite();
  doc::Image* image = site.image();
  if (!image)
    throw std::runtime_error("No active image/cel");
  return image;
}

json textResult(const json& payload)
{
  json item = json::object();
  item["type"] = "text";
  item["text"] = payload.dump(2);
  json result = json::object();
  result["content"] = json::array({item});
  return result;
}

json toolCreateSprite(const json& args)
{
  int w = args.value("width", 32);
  int h = args.value("height", 32);
  if (w < 1 || h < 1 || w > 65535 || h > 65535)
    throw std::runtime_error("width/height out of range");

  doc::PixelFormat format = parseColorMode(args);
  int ncolors = get_default_palette()->size();

  std::unique_ptr<doc::Sprite> sprite(
    doc::Sprite::createBasicSprite(format, w, h, ncolors));

  if (sprite->pixelFormat() != doc::IMAGE_GRAYSCALE)
    get_default_palette()->copyColorsTo(*sprite->palette(doc::frame_t(0)));

  static int spriteCounter = 0;
  std::unique_ptr<app::Document> doc(new app::Document(sprite.get()));
  sprite.release();
  char buf[64];
  std::snprintf(buf, sizeof(buf), "MCP-Sprite-%04d", ++spriteCounter);
  doc->setFilename(buf);
  doc->setContext(UIContext::instance());
  doc.release();

  if (auto* mgr = ui::Manager::getDefault())
    mgr->invalidate();

  json payload = json::object();
  payload["ok"] = true;
  payload["width"] = w;
  payload["height"] = h;
  payload["color_mode"] = colorModeName(format);
  payload["filename"] = buf;
  return textResult(payload);
}

json toolGetSpriteInfo(const json&)
{
  auto* doc = requireDocument();
  auto* spr = doc->sprite();
  json payload = json::object();
  payload["ok"] = true;
  payload["width"] = spr->width();
  payload["height"] = spr->height();
  payload["color_mode"] = colorModeName(spr->pixelFormat());
  payload["layers"] = static_cast<int>(spr->countLayers());
  payload["frames"] = static_cast<int>(spr->totalFrames());
  payload["filename"] = doc->filename();
  return textResult(payload);
}

json toolFillSprite(const json& args)
{
  app::Document* doc = nullptr;
  doc::Image* image = requireActiveImage(&doc);
  doc::color_t color = parseColor(args.at("color"), doc->sprite()->pixelFormat());
  doc::clear_image(image, color);
  doc->notifyGeneralUpdate();
  if (auto* mgr = ui::Manager::getDefault())
    mgr->invalidate();
  json payload = json::object();
  payload["ok"] = true;
  payload["action"] = "fill_sprite";
  return textResult(payload);
}

json toolDrawRect(const json& args)
{
  app::Document* doc = nullptr;
  doc::Image* image = requireActiveImage(&doc);
  int x = args.at("x").get<int>();
  int y = args.at("y").get<int>();
  int w = args.at("width").get<int>();
  int h = args.at("height").get<int>();
  if (w < 1 || h < 1)
    throw std::runtime_error("width/height must be >= 1");
  doc::color_t color = parseColor(args.at("color"), doc->sprite()->pixelFormat());
  doc::fill_rect(image, x, y, x + w - 1, y + h - 1, color);
  doc->notifyGeneralUpdate();
  if (auto* mgr = ui::Manager::getDefault())
    mgr->invalidate();
  json payload = json::object();
  payload["ok"] = true;
  payload["action"] = "draw_rect";
  payload["x"] = x;
  payload["y"] = y;
  payload["width"] = w;
  payload["height"] = h;
  return textResult(payload);
}

json toolSetPixels(const json& args)
{
  app::Document* doc = nullptr;
  doc::Image* image = requireActiveImage(&doc);
  if (!args.contains("pixels") || !args["pixels"].is_array())
    throw std::runtime_error("pixels must be an array");

  int count = 0;
  for (const auto& p : args["pixels"]) {
    int x = p.at("x").get<int>();
    int y = p.at("y").get<int>();
    doc::color_t color = parseColor(p.at("color"), doc->sprite()->pixelFormat());
    if (x >= 0 && y >= 0 && x < image->width() && y < image->height()) {
      doc::put_pixel(image, x, y, color);
      ++count;
    }
  }
  doc->notifyGeneralUpdate();
  if (auto* mgr = ui::Manager::getDefault())
    mgr->invalidate();
  json payload = json::object();
  payload["ok"] = true;
  payload["pixels_set"] = count;
  return textResult(payload);
}

json toolSaveSprite(const json& args)
{
  auto* doc = requireDocument();
  auto* ctx = UIContext::instance();
  std::string filename = args.value("path", args.value("filename", std::string()));
  if (filename.empty())
    throw std::runtime_error("path is required");

  doc->setFilename(filename);
  Params params;
  params.set("filename", filename.c_str());
  Command* cmd = CommandsModule::instance()->getCommandByName(CommandId::SaveFile);
  ctx->executeCommand(cmd, params);
  json payload = json::object();
  payload["ok"] = true;
  payload["path"] = filename;
  return textResult(payload);
}

json toolOpenSprite(const json& args)
{
  auto* ctx = UIContext::instance();
  std::string filename = args.value("path", args.value("filename", std::string()));
  if (filename.empty())
    throw std::runtime_error("path is required");

  Params params;
  params.set("filename", filename.c_str());
  Command* cmd = CommandsModule::instance()->getCommandByName(CommandId::OpenFile);
  ctx->executeCommand(cmd, params);

  auto* doc = ctx->activeDocument();
  json payload = json::object();
  payload["ok"] = true;
  payload["path"] = filename;
  payload["opened"] = doc != nullptr;
  payload["filename"] = doc ? doc->filename() : std::string();
  return textResult(payload);
}

json toolRunScript(const json& args)
{
  std::string code = args.value("code", args.value("script", std::string()));
  std::string path = args.value("path", args.value("filename", std::string()));

  bool ok = false;
  if (!path.empty() && code.empty()) {
    ok = AppScripting::evalFile(path);
  }
  else if (!code.empty()) {
    ok = AppScripting::eval(code, path.empty() ? "::mcp" : path);
  }
  else {
    throw std::runtime_error("Provide code or path");
  }

  if (auto* mgr = ui::Manager::getDefault())
    mgr->invalidate();

  json payload = json::object();
  payload["ok"] = ok;
  payload["action"] = "run_script";
  return textResult(payload);
}

json toolSchema(const char* name, const char* description, json schema)
{
  json out = json::object();
  out["name"] = name;
  out["description"] = description;
  out["inputSchema"] = std::move(schema);
  return out;
}

json objectSchema(std::initializer_list<std::pair<const char*, json>> props,
                  std::initializer_list<const char*> required = {})
{
  json properties = json::object();
  for (const auto& p : props)
    properties[p.first] = p.second;
  json schema = json::object();
  schema["type"] = "object";
  schema["properties"] = properties;
  if (required.size() > 0) {
    json req = json::array();
    for (const char* r : required)
      req.push_back(r);
    schema["required"] = req;
  }
  return schema;
}

} // namespace

json toolsList()
{
  json colorProp = json::object();
  colorProp["description"] = "#RRGGBB, #RRGGBBAA, {r,g,b,a}, or integer";

  json intProp = json::object();
  intProp["type"] = "integer";

  json strProp = json::object();
  strProp["type"] = "string";

  json colorModeProp = json::object();
  colorModeProp["type"] = "string";
  colorModeProp["description"] = "rgb | grayscale | indexed";

  json pixelsProp = json::object();
  pixelsProp["type"] = "array";
  pixelsProp["items"] = json::object({{"type", "object"}});

  json tools = json::array();
  tools.push_back(toolSchema(
    "create_sprite",
    "Create a new sprite document and make it active.",
    objectSchema({{"width", intProp}, {"height", intProp}, {"color_mode", colorModeProp}},
                 {"width", "height"})));
  tools.push_back(toolSchema(
    "get_sprite_info",
    "Return info about the active sprite.",
    objectSchema({})));
  tools.push_back(toolSchema(
    "fill_sprite",
    "Fill the active cel with a color (#RRGGBB / #RRGGBBAA / {r,g,b,a}).",
    objectSchema({{"color", colorProp}}, {"color"})));
  tools.push_back(toolSchema(
    "draw_rect",
    "Draw a filled rectangle on the active cel.",
    objectSchema({{"x", intProp}, {"y", intProp}, {"width", intProp},
                  {"height", intProp}, {"color", colorProp}},
                 {"x", "y", "width", "height", "color"})));
  tools.push_back(toolSchema(
    "set_pixels",
    "Set sparse pixels on the active cel. Each pixel: {x,y,color}.",
    objectSchema({{"pixels", pixelsProp}}, {"pixels"})));
  tools.push_back(toolSchema(
    "save_sprite",
    "Save the active sprite to a path.",
    objectSchema({{"path", strProp}}, {"path"})));
  tools.push_back(toolSchema(
    "open_sprite",
    "Open a sprite from a path.",
    objectSchema({{"path", strProp}}, {"path"})));
  tools.push_back(toolSchema(
    "run_script",
    "Execute JavaScript in LibreSprite (code string or path to .js file).",
    objectSchema({{"code", strProp}, {"path", strProp}})));

  json result = json::object();
  result["tools"] = tools;
  return result;
}

json callTool(const std::string& name, const json& args)
{
  json result;
  std::string error = callOnUiThread([&]{
    if (name == "create_sprite") result = toolCreateSprite(args);
    else if (name == "get_sprite_info") result = toolGetSpriteInfo(args);
    else if (name == "fill_sprite") result = toolFillSprite(args);
    else if (name == "draw_rect") result = toolDrawRect(args);
    else if (name == "set_pixels") result = toolSetPixels(args);
    else if (name == "save_sprite") result = toolSaveSprite(args);
    else if (name == "open_sprite") result = toolOpenSprite(args);
    else if (name == "run_script") result = toolRunScript(args);
    else throw std::runtime_error("Unknown tool: " + name);
  });

  if (!error.empty()) {
    return {
      {"content", json::array({
        {{"type", "text"}, {"text", json({{"ok", false}, {"error", error}}).dump()}}
      })},
      {"isError", true}
    };
  }
  return result;
}

} // namespace mcp
} // namespace app
