// LibreSprite
// Copyright (C) 2021-2026  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "delta/Extension.hpp"
#include "delta/JSON.hpp"
#include "di.hpp"
#include "app/script/api/script_api_common.h"

#include "app/cmd/clear_image.h"
#include "app/cmd/copy_region.h"
#include "app/cmd/set_pixel.h"
#include "app/document.h"
#include "app/script/app_scripting.h"
#include "app/transaction.h"
#include "app/ui_context.h"
#include "base/base64.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "gfx/rect.h"
#include "gfx/region.h"
#include "she/surface.h"
#include "she/system.h"
#include "ui/manager.h"

#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
  // The wrapper re-resolves the site's image on every call: a cel trim or a
  // canvas resize may replace (or remove) the image object underneath.
  doc::Image& resolveImage(script_api::ImageSite& site) {
    auto* img = site.image();
    if (!img)
      throw std::runtime_error{"No image at this site"};
    return *img;
  }

  // The script's Image is always a cel image (app.activeImage /
  // cel.image): the site's (document, layer, frame) slot resolves the
  // cel, and the cel's parent chain (cel -> layer -> sprite -> document)
  // gives the owning document.
  std::shared_ptr<const doc::Cel> imageCel(script_api::ImageSite& site) {
    auto cel = site.cel();
    if (!cel || !cel->image())
      throw std::runtime_error{"No image at this site"};
    return cel;
  }

  // Transaction for a raw image write: joins the script's active
  // app.transaction() scope when it is bound to the image's document
  // (the group commits as one undo step), otherwise makes the document
  // active (the sprite.save pattern) and owns a per-call Transaction
  // (one undo step per call). The own transaction is committed by
  // commit(); an escaping exception rolls it back (the Transaction dtor),
  // and the joined scope is committed by the scope itself.
  class ImageWriteTx {
  public:
    explicit ImageWriteTx(const std::shared_ptr<const doc::Cel>& cel)
      : m_doc(static_cast<app::Document*>(cel->document()))
      , m_tx(nullptr)
      , m_ownsTx(false)
    {
      if (!m_doc)
        throw std::runtime_error{"No document for this image"};
      app::Transaction* externalTx = app::AppScripting::activeTransaction();
      if (externalTx && app::AppScripting::activeTransactionDocument() != m_doc)
        throw std::runtime_error{
          "the image's document is not the document the script's "
          "transaction scope is bound to (the scope is bound to the "
          "document active when app.transaction started)"};
      if (externalTx) {
        m_tx = externalTx;
      } else {
        auto* ctx = app::UIContext::instance();
        if (!ctx)
          throw std::runtime_error{"No UI context is available"};
        // The Transaction binds to the context's active document's
        // undo history: make the image's document active first (the
        // same side effect as sprite.save).
        ctx->setActiveDocument(m_doc);
        m_ownTx.reset(new app::Transaction(ctx, "Script Execution", app::ModifyDocument));
        m_tx = m_ownTx.get();
        m_ownsTx = true;
      }
    }

    app::Transaction& tx() { return *m_tx; }
    app::Document* document() const { return m_doc; }
    void commit() {
      if (m_ownsTx) {
        m_tx->commit();
        m_ownsTx = false;
      }
    }

  private:
    app::Document* m_doc;
    std::unique_ptr<app::Transaction> m_ownTx;
    app::Transaction* m_tx;
    bool m_ownsTx;
  };
} // namespace

class ImageExtension : public Extension {
public:
  ImageExtension() {
    using namespace script_api;
    auto& clazz = addClass<void, ImageSite>("Image");
    clazz.setConstructor() = []() -> std::shared_ptr<void> {
      throw std::runtime_error{"Image cannot be constructed directly"};
    };

    clazz.addGetter("width") = [](ImageSite& site) -> JSON::Value {
      return (double)resolveImage(site).width();
    };
    clazz.addGetter("height") = [](ImageSite& site) -> JSON::Value {
      return (double)resolveImage(site).height();
    };
    clazz.addGetter("stride") = [](ImageSite& site) -> JSON::Value {
      return (double)resolveImage(site).getRowStrideSize();
    };
    clazz.addGetter("format") = [](ImageSite& site) -> JSON::Value {
      return (double)resolveImage(site).pixelFormat();
    };

    clazz.addMethod("getPixel") = [](ImageSite& site, double x, double y) -> JSON::Value {
      return (double)resolveImage(site).getPixel((int)x, (int)y);
    };

    clazz.addMethod("putPixel") = [](ImageSite& site, double x, double y, double color) -> JSON::Value {
      auto& img = resolveImage(site);
      if ((unsigned)x < (unsigned)img.width() && (unsigned)y < (unsigned)img.height()) {
        const int px = (int)x, py = (int)y;
        auto cel = imageCel(site);
        ImageWriteTx tx(cel);
        tx.tx().execute(new app::cmd::SetPixel(&img, px, py, (doc::color_t)color));
        tx.commit();
        // The tool loop's equivalent is updateDirtyArea: the region is
        // sprite-space (the cel may be offset after a trim).
        tx.document()->notifySpritePixelsModified(
          cel->sprite(),
          gfx::Region(gfx::Rect(px + cel->x(), py + cel->y(), 1, 1)),
          cel->frame());
      }
      return {};
    };

    clazz.addMethod("clear") = [](ImageSite& site, double color) -> JSON::Value {
      auto& img = resolveImage(site);
      auto cel = imageCel(site);
      ImageWriteTx tx(cel);
      tx.tx().execute(new app::cmd::ClearImage(&img, (doc::color_t)color));
      tx.commit();
      tx.document()->notifySpritePixelsModified(
        cel->sprite(),
        gfx::Region(gfx::Rect(cel->x(), cel->y(), img.width(), img.height())),
        cel->frame());
      return {};
    };

    clazz.addMethod("putImageData") = [](ImageSite& site, JSON::Value& data) -> JSON::Value {
      auto& img = resolveImage(site);
      auto& bytes = data.byteArray();
      if (bytes.size() != std::size_t(img.getRowStrideSize()) * img.height()) {
        std::cout << "Data size mismatch: " << bytes.size() << std::endl;
        return {};
      }
      auto cel = imageCel(site);
      // The script bytes go into a temporary image (one allocation per
      // call); CopyRegion::onExecute writes them into the cel image and
      // captures the old bytes (immediate getPixel visibility + undo).
      auto temp = std::shared_ptr<doc::Image>(
        doc::Image::create(img.pixelFormat(), img.width(), img.height()));
      std::memcpy(temp->getPixelAddress(0, 0), bytes.data(), bytes.size());
      ImageWriteTx tx(cel);
      tx.tx().execute(new app::cmd::CopyRegion(&img, temp.get(),
        gfx::Region(img.bounds()), gfx::Point(0, 0)));
      tx.commit();
      tx.document()->notifySpritePixelsModified(
        cel->sprite(),
        gfx::Region(gfx::Rect(cel->x(), cel->y(), img.width(), img.height())),
        cel->frame());
      if (auto* mgr = ui::Manager::getDefault())
        mgr->invalidate();
      return {};
    };

    clazz.addMethod("getImageData") = [](ImageSite& site) -> JSON::Value {
      auto& img = resolveImage(site);
      auto* addr = img.getPixelAddress(0, 0);
      std::size_t size = std::size_t(img.getRowStrideSize()) * img.height();
      auto vec = std::make_shared<std::vector<uint8_t>>(addr, addr + size);
      return JSON::Value{vec};
    };

    clazz.addMethod("getPNGData") = [](ImageSite& site) -> JSON::Value {
      auto& img = resolveImage(site);
      auto w = img.width();
      auto h = img.height();
      std::shared_ptr<she::Surface> surface{
        she::instance()->createRgbaSurface(w, h),
        [](she::Surface* s) { s->dispose(); }
      };
      if (!surface)
        return std::string{};

      for (auto y = 0; y < h; ++y)
        for (auto x = 0; x < w; ++x)
          surface->putPixel(img.getPixel(x, y), x, y);

      auto png = she::instance()->encodeSurfaceAsPNG(surface.get());
      if (png.empty())
        throw std::runtime_error{"Failed to encode PNG data"};
      std::string encoded;
      base::encode_base64(png, encoded);
      return std::string{"data:image/png;base64,"} + encoded;
    };
  }
};

static di::provide<Extension, ImageExtension> x{"image"};
