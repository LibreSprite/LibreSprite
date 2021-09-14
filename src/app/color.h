// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "doc/color.h"
#include "doc/pixel_format.h"

#include <string>

namespace doc {
  class Image;
  class Layer;
}

namespace app {

  using namespace doc;

  class Color {
  public:
    enum Type {
      MaskType,
      RgbType,
      HsvType,
      GrayType,
      IndexType,
    };

    enum HumanReadableString {
      ShortHumanReadableString,
      LongHumanReadableString
    };

    // Default ctor is mask color
    Color() : m_type(MaskType) { }

    static Color fromMask();
    static Color fromRgb(int r, int g, int b, int a = 255);
    static Color fromHsv(double h, double s, double v, int a = 255); // h=[0,360], s=[0,100], v=[0,100]
    static Color fromGray(int g, int a = 255);
    static Color fromIndex(int index);

    static Color fromImage(PixelFormat pixelFormat, color_t c);
    static Color fromImageGetPixel(Image* image, int x, int y);
    static Color fromString(const std::string& str);

    std::string toString() const;
    std::string toHumanReadableString(PixelFormat format, HumanReadableString type) const;

    bool operator==(const Color& other) const;
    bool operator!=(const Color& other) const {
      return !operator==(other);
    }

    Type getType() const {
      return m_type;
    }

    bool isValid() const;

    // Getters
    int getRed() const;
    int getGreen() const;
    int getBlue() const;
    double getHue() const;
    double getSaturation() const;
    double getValue() const;
    int getGray() const;
    int getIndex() const;
    int getAlpha() const;

  private:
    Color(Type type) : m_type(type) { }

    // Color type
    Type m_type;

    // Color value
    union {
      struct {
        int r, g, b, a;
      } rgb;
      struct {
        double h, s, v;
        int a;
      } hsv;
      struct {
        int g, a;
      } gray;
      int index;
    } m_value;
  };

} // namespace app
