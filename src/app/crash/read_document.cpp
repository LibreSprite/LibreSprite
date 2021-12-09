// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/crash/read_document.h"

#include "app/console.h"
#include "app/crash/internals.h"
#include "app/document.h"
#include "base/convert_to.h"
#include "base/exception.h"
#include "base/fs.h"
#include "base/fstream_path.h"
#include "base/path.h"
#include "base/serialization.h"
#include "base/string.h"
#include "doc/cel.h"
#include "doc/cel_data_io.h"
#include "doc/cel_io.h"
#include "doc/cels_range.h"
#include "doc/frame.h"
#include "doc/frame_tag.h"
#include "doc/frame_tag_io.h"
#include "doc/image_io.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/palette_io.h"
#include "doc/sprite.h"
#include "doc/string_io.h"
#include "doc/subobjects_io.h"

#include <fstream>
#include <map>
#include <memory>

namespace app {
namespace crash {

using namespace base::serialization;
using namespace base::serialization::little_endian;
using namespace doc;

namespace {

class Reader : public SubObjectsIO {
public:
  Reader(const std::string& dir)
    : m_sprite(nullptr)
    , m_dir(dir)
    , m_docId(0)
    , m_docVersions(nullptr)
    , m_loadInfo(nullptr) {
    for (const auto& fn : base::list_files(dir)) {
      auto i = fn.find('-');
      if (i == std::string::npos)
        continue;               // Has no ID

      auto j = fn.find('.', ++i);
      if (j == std::string::npos)
        continue;               // Has no version

      ObjectId id = base::convert_to<int>(fn.substr(i, j - i));
      ObjectVersion ver = base::convert_to<int>(fn.substr(j+1));
      if (!id || !ver)
        continue;               // Error converting strings to ID/ver

      ObjVersions& versions = m_objVersions[id];
      versions.add(ver);

      if (fn.compare(0, 3, "doc") == 0) {
        if (!m_docId)
          m_docId = id;
        else {
          ASSERT(m_docId == id);
        }

        m_docVersions = &versions;
      }
    }
  }

  app::Document* loadDocument() {
    app::Document* doc = loadObject<app::Document*>("doc", m_docId, &Reader::readDocument);
    if (doc)
      fixUndetectedDocumentIssues(doc);
    else
      Console().printf("Error recovering the document\n");
    return doc;
  }

  bool loadDocumentInfo(DocumentInfo& info) {
    m_loadInfo = &info;
    return
      loadObject<app::Document*>("doc", m_docId, &Reader::readDocument)
      == (app::Document*)1;
  }

private:

  const ObjectVersion docId() const {
    return m_docId;
  }

  const ObjVersions* docVersions() const {
    return m_docVersions;
  }

  Sprite* loadSprite(ObjectId sprId) {
    return loadObject<Sprite*>("spr", sprId, &Reader::readSprite);
  }

  std::shared_ptr<Image> getImageRef(ObjectId imageId) {
    if (m_images.find(imageId) != m_images.end())
      return m_images[imageId];

    std::shared_ptr<Image> image(loadObject<Image*>("img", imageId, &Reader::readImage));
    return m_images[imageId] = image;
  }

  CelDataRef getCelDataRef(ObjectId celdataId) {
    if (m_celdatas.find(celdataId) != m_celdatas.end())
      return m_celdatas[celdataId];

    CelDataRef celData(loadObject<CelData*>("celdata", celdataId, &Reader::readCelData));
    return m_celdatas[celdataId] = celData;
  }

  template<typename T>
  T loadObject(const char* prefix, ObjectId id, T (Reader::*readMember)(std::ifstream&)) {
    const ObjVersions& versions = m_objVersions[id];

    for (size_t i=0; i<versions.size(); ++i) {
      ObjectVersion ver = versions[i];
      if (!ver)
        continue;

      TRACE(" - Restoring %s #%d v%d\n", prefix, id, ver);

      std::string fn = prefix;
      fn.push_back('-');
      fn += base::convert_to<std::string>(id);
      fn.push_back('.');
      fn += base::convert_to<std::string>(ver);

      std::ifstream s(FSTREAM_PATH(base::join_path(m_dir, fn)), std::ifstream::binary);
      T obj = nullptr;
      if (read32(s) == MAGIC_NUMBER)
        obj = (this->*readMember)(s);

      if (obj) {
        TRACE(" - %s #%d v%d restored successfully\n", prefix, id, ver);
        return obj;
      }
      else {
        TRACE(" - %s #%d v%d was not restored\n", prefix, id, ver);
      }
    }

    // Show error only if we've failed to load all versions
    if (!m_loadInfo)
      Console().printf("Error loading object %s #%d\n", prefix, id);

    return nullptr;
  }

  app::Document* readDocument(std::ifstream& s) {
    ObjectId sprId = read32(s);
    std::string filename = read_string(s);

    // Load DocumentInfo only
    if (m_loadInfo) {
      m_loadInfo->filename = filename;
      return (app::Document*)loadSprite(sprId);
    }

    Sprite* spr = loadSprite(sprId);
    if (spr) {
      app::Document* doc = new app::Document(spr);
      doc->setFilename(filename);
      doc->impossibleToBackToSavedState();
      return doc;
    }
    else {
      Console().printf("Unable to load sprite #%d\n", sprId);
      return nullptr;
    }
  }

  Sprite* readSprite(std::ifstream& s) {
    PixelFormat format = (PixelFormat)read8(s);
    int w = read16(s);
    int h = read16(s);
    color_t transparentColor = read32(s);
    frame_t nframes = read32(s);

    if (format != IMAGE_RGB &&
        format != IMAGE_INDEXED &&
        format != IMAGE_GRAYSCALE) {
      if (!m_loadInfo)
        Console().printf("Invalid sprite format #%d\n", (int)format);
      return nullptr;
    }

    if (w < 1 || h < 1 || w > 0xfffff || h > 0xfffff) {
      if (!m_loadInfo)
        Console().printf("Invalid sprite dimension %dx%d\n", w, h);
      return nullptr;
    }

    if (m_loadInfo) {
      m_loadInfo->format = format;
      m_loadInfo->width = w;
      m_loadInfo->height = w;
      m_loadInfo->frames = nframes;
      return (Sprite*)1;
    }

    std::unique_ptr<Sprite> spr(new Sprite(format, w, h, 256));
    m_sprite = spr.get();
    spr->setTransparentColor(transparentColor);

    if (nframes >= 1) {
      spr->setTotalFrames(nframes);
      for (frame_t fr=0; fr<nframes; ++fr) {
        int msecs = read32(s);
        spr->setFrameDuration(fr, msecs);
      }
    }
    else {
      Console().printf("Invalid number of frames #%d\n", nframes);
    }

    // Read layers
    int nlayers = read32(s);
    if (nlayers >= 1 && nlayers < 0xfffff) {
      for (int i = 0; i < nlayers; ++i) {
        ObjectId layId = read32(s);
        Layer* lay = loadObject<Layer*>("lay", layId, &Reader::readLayer);
        if (lay)
          spr->folder()->addLayer(lay);
      }
    }
    else {
      Console().printf("Invalid number of layers #%d\n", nlayers);
    }

    // Read palettes
    int npalettes = read32(s);
    if (npalettes >= 1 && npalettes < 0xfffff) {
      for (int i = 0; i < npalettes; ++i) {
        ObjectId palId = read32(s);
        Palette* pal = loadObject<Palette*>("pal", palId, &Reader::readPalette);
        if (pal)
          spr->setPalette(pal, true);
      }
    }

    // Read frame tags
    int nfrtags = read32(s);
    if (nfrtags >= 1 && nfrtags < 0xfffff) {
      for (int i = 0; i < nfrtags; ++i) {
        ObjectId tagId = read32(s);
        FrameTag* tag = loadObject<FrameTag*>("frtag", tagId, &Reader::readFrameTag);
        if (tag)
          spr->frameTags().add(tag);
      }
    }

    return spr.release();
  }

  Layer* readLayer(std::ifstream& s) {
    LayerFlags flags = (LayerFlags)read32(s);
    ObjectType type = (ObjectType)read16(s);
    ASSERT(type == ObjectType::LayerImage);

    std::string name = read_string(s);

    if (type == ObjectType::LayerImage) {
      std::unique_ptr<LayerImage> lay(new LayerImage(m_sprite));
      lay->setName(name);
      lay->setFlags(flags);

      // Cels
      int ncels = read32(s);
      for (int i=0; i<ncels; ++i) {
        ObjectId celId = read32(s);
        Cel* cel = loadObject<Cel*>("cel", celId, &Reader::readCel);
        if (cel) {
          // Expand sprite size
          if (cel->frame() > m_sprite->lastFrame())
            m_sprite->setTotalFrames(cel->frame()+1);

          lay->addCel(cel);
        }
      }
      return lay.release();
    }
    else {
      Console().printf("Unable to load layer named '%s', type #%d\n",
        name.c_str(), (int)type);
      return nullptr;
    }
  }

  Cel* readCel(std::ifstream& s) {
    return read_cel(s, this, false);
  }

  CelData* readCelData(std::ifstream& s) {
    return read_celdata(s, this, false);
  }

  Image* readImage(std::ifstream& s) {
    return read_image(s, false);
  }

  Palette* readPalette(std::ifstream& s) {
    return read_palette(s);
  }

  FrameTag* readFrameTag(std::ifstream& s) {
    return read_frame_tag(s, false);
  }

  // Fix issues that the restoration process could produce.
  void fixUndetectedDocumentIssues(app::Document* doc) {
    Sprite* spr = doc->sprite();
    ASSERT(spr);
    if (!spr)
      return;                   // TODO create an empty sprite

    // Fill the background layer with empty cels if they are missing
    if (LayerImage* bg = spr->backgroundLayer()) {
      for (frame_t fr=0; fr<spr->totalFrames(); ++fr) {
        Cel* cel = bg->cel(fr);
        if (!cel) {
          std::shared_ptr<Image> image(Image::create(spr->pixelFormat(),
                                       spr->width(),
                                       spr->height()));
          image->clear(spr->transparentColor());
          cel = new Cel(fr, image);
          bg->addCel(cel);
        }
      }
    }
  }

  Sprite* m_sprite;    // Used to pass the sprite in LayerImage() ctor
  std::string m_dir;
  ObjectVersion m_docId;
  ObjVersionsMap m_objVersions;
  ObjVersions* m_docVersions;
  DocumentInfo* m_loadInfo;
  std::map<ObjectId, std::shared_ptr<Image>> m_images;
  std::map<ObjectId, CelDataRef> m_celdatas;
};

} // anonymous namespace

//////////////////////////////////////////////////////////////////////
// Public API

bool read_document_info(const std::string& dir, DocumentInfo& info)
{
  return Reader(dir).loadDocumentInfo(info);
}

app::Document* read_document(const std::string& dir)
{
  return Reader(dir).loadDocument();
}

app::Document* read_document_with_raw_images(const std::string& dir,
                                             RawImagesAs as)
{
  Reader reader(dir);

  DocumentInfo info;
  if (!reader.loadDocumentInfo(info)) {
    info.format = IMAGE_RGB;
    info.width = 256;
    info.height = 256;
    info.filename = "Unknown";
  }
  info.width = MID(1, info.width, 99999);
  info.height = MID(1, info.height, 99999);
  Sprite* spr = new Sprite(info.format, info.width, info.height, 256);

  // Load each image as a new frame
  auto lay = new LayerImage(spr);
  spr->folder()->addLayer(lay);

  frame_t frame = 0;
  for (const auto& fn : base::list_files(dir)) {
    if (fn.compare(0, 3, "img") != 0)
      continue;

    std::ifstream s(FSTREAM_PATH(base::join_path(dir, fn)), std::ifstream::binary);
    if (!s)
      continue;

    std::shared_ptr<Image> img;
    if (read32(s) == MAGIC_NUMBER)
      img.reset(read_image(s, false));

    if (img) {
      lay->addCel(new Cel(frame, img));
    }

    switch (as) {
      case RawImagesAs::kFrames:
        ++frame;
        break;
      case RawImagesAs::kLayers:
        lay = new LayerImage(spr);
        spr->folder()->addLayer(lay);
        break;
    }
  }
  if (as == RawImagesAs::kFrames) {
    if (frame > 1)
      spr->setTotalFrames(frame);
  }

  app::Document* doc = new app::Document(spr);
  doc->setFilename(info.filename);
  doc->impossibleToBackToSavedState();
  return doc;
}

} // namespace crash
} // namespace app
