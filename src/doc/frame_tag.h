// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "base/disable_copying.h"
#include "doc/anidir.h"
#include "doc/color.h"
#include "doc/frame.h"
#include "doc/object.h"

#include <string>

namespace doc {
  class FrameTags;

  class FrameTag : public Object {
  public:
    FrameTag(frame_t from, frame_t to);
    FrameTag(const FrameTag& other);
    ~FrameTag();

    FrameTags* owner() const { return m_owner; }
    frame_t fromFrame() const { return m_from; }
    frame_t toFrame() const { return m_to; }
    const std::string& name() const { return m_name; }
    color_t color() const { return m_color; }
    AniDir aniDir() const { return m_aniDir; }

    void setFrameRange(frame_t from, frame_t to);
    void setName(const std::string& name);
    void setColor(color_t color);
    void setAniDir(AniDir aniDir);

    void setOwner(FrameTags* owner);

  public:
    FrameTags* m_owner;
    frame_t m_from, m_to;
    color_t m_color;
    std::string m_name;
    AniDir m_aniDir;

    // Disable operator=
    FrameTag& operator=(FrameTag&);
  };

} // namespace doc
