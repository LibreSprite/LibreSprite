// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "base/debug.h"
#include "doc/color.h"
#include "doc/frame.h"
#include "doc/object.h"

#include <vector>
#include <string>

namespace doc {

  class Remap;

  class Palette : public Object {
  public:
    Palette(frame_t frame, int ncolors);
    Palette(const Palette& palette);
    Palette(const Palette& palette, const Remap& remap);
    ~Palette();

    static Palette* createGrayscale();

    int size() const { return (int)m_colors.size(); }
    void resize(int ncolors);

    std::string filename() const { return m_filename; }
    void setFilename(const std::string& filename) {
      m_filename = filename;
    }

    int getModifications() const { return m_modifications; }

    // Return true if the palette has alpha != 255 in some entry
    bool hasAlpha() const;

    frame_t frame() const { return m_frame; }
    void setFrame(frame_t frame);

    color_t entry(int i) const {
      // TODO At this moment we cannot enable this assert because
      //      there are situations that are not handled quite well yet.
      //      E.g. A palette with lesser colors is loaded
      //
      //ASSERT(i < size());
      ASSERT(i >= 0);
      if (i >= 0 && i < size())
        return m_colors[i];
      else
        return 0;
    }
    color_t getEntry(int i) const {
      return entry(i);
    }
    void setEntry(int i, color_t color);
    void addEntry(color_t color);

    void copyColorsTo(Palette* dst) const;

    int countDiff(const Palette* other, int* from, int* to) const;

    bool operator==(const Palette& other) const {
      return (countDiff(&other, nullptr, nullptr) == 0);
    }

    bool operator!=(const Palette& other) const {
      return !operator==(other);
    }

    // Returns true if the palette is completelly black.
    bool isBlack() const;
    void makeBlack();

    void makeGradient(int from, int to);

    int findExactMatch(int r, int g, int b, int a, int mask_index) const;
    int findBestfit(int r, int g, int b, int a, int mask_index) const;

    void applyRemap(const Remap& remap);

  private:
    frame_t m_frame;
    std::vector<color_t> m_colors;
    int m_modifications;
    std::string m_filename; // If the palette is associated with a file.
  };

} // namespace doc
