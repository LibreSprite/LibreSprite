// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "base/debug.h"
#include "doc/image.h"
#include "doc/object.h"
#include "doc/with_user_data.h"

#include <memory>

namespace doc {

  class CelData : public WithUserData {
  public:
    CelData(const std::shared_ptr<Image>& image);
    CelData(const CelData& celData);

    const gfx::Point& position() const { return m_position; }
    int opacity() const { return m_opacity; }
    Image* image() const { return const_cast<Image*>(m_image.get()); };
    std::shared_ptr<Image> imageRef() const { return m_image; }

    void setImage(const std::shared_ptr<Image>& image);
    void setPosition(int x, int y) {
      m_position.x = x;
      m_position.y = y;
    }
    void setPosition(const gfx::Point& pos) { m_position = pos; }
    void setOpacity(int opacity) { m_opacity = opacity; }

    virtual int getMemSize() const override {
      ASSERT(m_image);
      return sizeof(CelData) + m_image->getMemSize();
    }

  private:
    std::shared_ptr<Image> m_image;
    gfx::Point m_position;      // X/Y screen position
    int m_opacity;              // Opacity level
  };

  typedef std::shared_ptr<CelData> CelDataRef;

} // namespace doc
