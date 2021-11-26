// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "base/disable_copying.h"
#include "doc/cel_data.h"
#include "doc/frame.h"
#include "doc/image.h"
#include "doc/object.h"
#include "gfx/fwd.h"
#include "gfx/point.h"

namespace doc {

  class Document;
  class LayerImage;
  class Sprite;

  class Cel : public Object {
  public:
    Cel(frame_t frame, const std::shared_ptr<Image>& image);
    Cel(frame_t frame, const CelDataRef& celData);

    static Cel* createCopy(const Cel* other);
    static Cel* createLink(const Cel* other);

    frame_t frame() const { return m_frame; }
    int x() const { return m_data->position().x; }
    int y() const { return m_data->position().y; }
    const gfx::Point& position() const { return m_data->position(); }
    int opacity() const { return m_data->opacity(); }

    LayerImage* layer() const { return m_layer; }
    Image* image() const { return m_data->image(); }
    std::shared_ptr<Image> imageRef() const { return m_data->imageRef(); }
    CelData* data() const { return const_cast<CelData*>(m_data.get()); }
    CelDataRef dataRef() const { return m_data; }
    Document* document() const;
    Sprite* sprite() const;
    Cel* link() const;
    std::size_t links() const;
    gfx::Rect bounds() const;

    // You should change the frame only if the cel isn't member of a
    // layer. If the cel is already in a layer, you should use
    // LayerImage::moveCel() member function.
    void setFrame(frame_t frame);
    void setDataRef(const CelDataRef& celData);
    void setPosition(int x, int y);
    void setPosition(const gfx::Point& pos);
    void setOpacity(int opacity);

    virtual int getMemSize() const override {
      return sizeof(Cel) + m_data->getMemSize();
    }

    void setParentLayer(LayerImage* layer);

  private:
    void fixupImage();

    LayerImage* m_layer;
    frame_t m_frame;            // Frame position
    CelDataRef m_data;

    Cel();
    DISABLE_COPYING(Cel);
  };

} // namespace doc
