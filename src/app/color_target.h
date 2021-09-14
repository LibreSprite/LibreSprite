// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "doc/color.h"
#include "doc/layer.h"
#include "doc/pixel_format.h"
#include "doc/sprite.h"

namespace app {

  // Represents the kind of surface where we'll use a color.
  class ColorTarget {
  public:
    enum LayerType {
      BackgroundLayer,
      TransparentLayer
    };

    ColorTarget(LayerType layerType, doc::PixelFormat pixelFormat, doc::color_t maskColor) :
      m_layerType(layerType),
      m_pixelFormat(pixelFormat),
      m_maskColor(maskColor) {
    }

    ColorTarget(doc::Layer* layer) :
      m_layerType(layer->isBackground() ? BackgroundLayer: TransparentLayer),
      m_pixelFormat(layer->sprite()->pixelFormat()),
      m_maskColor(layer->sprite()->transparentColor()) {
    }

    bool isBackground() const { return m_layerType == BackgroundLayer; }
    bool isTransparent() const { return m_layerType == TransparentLayer; }
    LayerType layerType() const { return m_layerType; }
    doc::PixelFormat pixelFormat() const { return m_pixelFormat; }
    doc::color_t maskColor() const { return m_maskColor; }

  private:
    LayerType m_layerType;
    doc::PixelFormat m_pixelFormat;
    doc::color_t m_maskColor;
  };

} // namespace app
