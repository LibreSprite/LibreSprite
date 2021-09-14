// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/sprite_sheet_type.h"
#include "base/disable_copying.h"
#include "doc/image_buffer.h"
#include "gfx/fwd.h"

#include <iosfwd>
#include <string>
#include <vector>

namespace doc {
  class FrameTag;
  class Image;
  class Layer;
}

namespace app {
  class Document;

  class DocumentExporter {
  public:
    enum DataFormat {
      JsonHashDataFormat,
      JsonArrayDataFormat,
      DefaultDataFormat = JsonHashDataFormat
    };

    enum TextureFormat {
      JsonTextureFormat,
      DefaultTextureFormat = JsonTextureFormat
    };

    enum ScaleMode {
      DefaultScaleMode
    };

    DocumentExporter();

    void setDataFormat(DataFormat format) { m_dataFormat = format; }
    void setDataFilename(const std::string& filename) { m_dataFilename = filename; }
    void setTextureFormat(TextureFormat format) { m_textureFormat = format; }
    void setTextureFilename(const std::string& filename) { m_textureFilename = filename; }
    void setTextureWidth(int width) { m_textureWidth = width; }
    void setTextureHeight(int height) { m_textureHeight = height; }
    void setSpriteSheetType(SpriteSheetType type) { m_sheetType = type; }
    void setScale(double scale) { m_scale = scale; }
    void setScaleMode(ScaleMode mode) { m_scaleMode = mode; }
    void setIgnoreEmptyCels(bool ignore) { m_ignoreEmptyCels = ignore; }
    void setBorderPadding(int padding) { m_borderPadding = padding; }
    void setShapePadding(int padding) { m_shapePadding = padding; }
    void setInnerPadding(int padding) { m_innerPadding = padding; }
    void setTrimCels(bool trim) { m_trimCels = trim; }
    void setFilenameFormat(const std::string& format) { m_filenameFormat = format; }
    void setListFrameTags(bool value) { m_listFrameTags = value; }
    void setListLayers(bool value) { m_listLayers = value; }

    void addDocument(Document* document,
                     doc::Layer* layer = nullptr,
                     doc::FrameTag* tag = nullptr,
                     bool temporalTag = false) {
      m_documents.push_back(Item(document, layer, tag, temporalTag));
    }

    Document* exportSheet();

  private:
    class Sample;
    class Samples;
    class LayoutSamples;
    class SimpleLayoutSamples;
    class BestFitLayoutSamples;

    void captureSamples(Samples& samples);
    Document* createEmptyTexture(const Samples& samples);
    void renderTexture(const Samples& samples, doc::Image* textureImage);
    void createDataFile(const Samples& samples, std::ostream& os, doc::Image* textureImage);
    void renderSample(const Sample& sample, doc::Image* dst, int x, int y);

    class Item {
    public:
      Document* doc;
      doc::Layer* layer;
      doc::FrameTag* frameTag;
      bool temporalTag;

      Item(Document* doc,
           doc::Layer* layer,
           doc::FrameTag* frameTag,
           bool temporalTag)
        : doc(doc), layer(layer), frameTag(frameTag)
        , temporalTag(temporalTag) {
      }

      int frames() const;
      int fromFrame() const;
      int toFrame() const;
    };
    typedef std::vector<Item> Items;

    DataFormat m_dataFormat;
    std::string m_dataFilename;
    TextureFormat m_textureFormat;
    std::string m_textureFilename;
    int m_textureWidth;
    int m_textureHeight;
    SpriteSheetType m_sheetType;
    double m_scale;
    ScaleMode m_scaleMode;
    bool m_ignoreEmptyCels;
    int m_borderPadding;
    int m_shapePadding;
    int m_innerPadding;
    bool m_trimCels;
    Items m_documents;
    std::string m_filenameFormat;
    doc::ImageBufferPtr m_sampleRenderBuf;
    bool m_listFrameTags;
    bool m_listLayers;

    DISABLE_COPYING(DocumentExporter);
  };

} // namespace app
