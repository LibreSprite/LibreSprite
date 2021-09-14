// Aseprite FreeType Wrapper
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "base/disable_copying.h"
#include "base/string.h"
#include "ft/freetype_headers.h"
#include "gfx/rect.h"

#include <map>

namespace ft {

  struct Glyph {
    FT_UInt glyph_index;
    FT_Glyph ft_glyph;
    FT_Bitmap* bitmap;
    double bearingX;
    double bearingY;
    double x;
    double y;
  };

  template<typename Cache>
  class FaceBase {
  public:
    FaceBase(FT_Face face) : m_face(face) {
    }

    ~FaceBase() {
      if (m_face)
        FT_Done_Face(m_face);
    }

    operator FT_Face() { return m_face; }
    FT_Face operator->() { return m_face; }

    bool isValid() const {
      return (m_face != nullptr);
    }

    bool antialias() const {
      return m_antialias;
    }

    void setAntialias(bool antialias) {
      m_antialias = antialias;
      m_cache.invalidate();
    }

    void setSize(int size) {
      FT_Set_Pixel_Sizes(m_face, size, size);
      m_cache.invalidate();
    }

    double height() const {
      FT_Size_Metrics* metrics = &m_face->size->metrics;
      double em_size = 1.0 * m_face->units_per_EM;
      double y_scale = metrics->y_ppem / em_size;
      return int(m_face->height * y_scale) - 1;
    }

    double ascender() const {
      FT_Size_Metrics* metrics = &m_face->size->metrics;
      double em_size = 1.0 * m_face->units_per_EM;
      double y_scale = metrics->y_ppem / em_size;
      return int(m_face->ascender * y_scale);
    }

    double descender() const {
      FT_Size_Metrics* metrics = &m_face->size->metrics;
      double em_size = 1.0 * m_face->units_per_EM;
      double y_scale = metrics->y_ppem / em_size;
      return int(m_face->descender * y_scale);
    }

  protected:
    FT_Face m_face;
    bool m_antialias;
    Cache m_cache;

  private:
    DISABLE_COPYING(FaceBase);
  };

  template<typename Cache>
  class FaceFT : public FaceBase<Cache> {
  public:
    using FaceBase<Cache>::m_face;
    using FaceBase<Cache>::m_cache;

    FaceFT(FT_Face face)
      : FaceBase<Cache>(face) {
    }

    template<typename Callback>
    void forEachGlyph(const std::string& str, Callback callback) {
      bool use_kerning = (FT_HAS_KERNING(this->m_face) ? true: false);
      FT_UInt prev_glyph = 0;
      double x = 0, y = 0;

      auto it = base::utf8_const_iterator(str.begin());
      auto end = base::utf8_const_iterator(str.end());
      for (; it != end; ++it) {
        FT_UInt glyph_index = this->m_cache.getGlyphIndex(
          this->m_face, *it);

        if (use_kerning && prev_glyph && glyph_index) {
          FT_Vector kerning;
          FT_Get_Kerning(this->m_face, prev_glyph, glyph_index,
                         FT_KERNING_DEFAULT, &kerning);
          x += kerning.x / 64.0;
        }

        Glyph* glyph = this->m_cache.loadGlyph(
          this->m_face, glyph_index, this->m_antialias);
        if (glyph) {
          glyph->bitmap = &FT_BitmapGlyph(glyph->ft_glyph)->bitmap;
          glyph->x = x + glyph->bearingX;
          glyph->y = y
            + this->height()
            + this->descender() // descender is negative
            - glyph->bearingY;

          callback(*glyph);

          x += glyph->ft_glyph->advance.x / double(1 << 16);
          y += glyph->ft_glyph->advance.y / double(1 << 16);

          this->m_cache.doneGlyph(glyph);
        }

        prev_glyph = glyph_index;
      }
    }

    gfx::Rect calcTextBounds(const std::string& str) {
      gfx::Rect bounds(0, 0, 0, 0);

      forEachGlyph(
        str,
        [&bounds, this](Glyph& glyph) {
          bounds |= gfx::Rect(int(glyph.x),
                              int(glyph.y),
                              glyph.bitmap->width,
                              glyph.bitmap->rows);
        });

      return bounds;
    }
  };

  class NoCache {
  public:
    void invalidate() {
      // Do nothing
    }

    FT_UInt getGlyphIndex(FT_Face face, int charCode) {
      return FT_Get_Char_Index(face, charCode);
    }

    Glyph* loadGlyph(FT_Face face, FT_UInt glyphIndex, bool antialias) {
      FT_Error err = FT_Load_Glyph(
        face, glyphIndex,
        FT_LOAD_RENDER |
        (antialias ? FT_LOAD_TARGET_NORMAL:
                     FT_LOAD_TARGET_MONO));
      if (err)
        return nullptr;

      FT_Glyph ft_glyph;
      err = FT_Get_Glyph(face->glyph, &ft_glyph);
      if (err)
        return nullptr;

      if (ft_glyph->format != FT_GLYPH_FORMAT_BITMAP) {
        err = FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, 0, 1);
        if (!err) {
          FT_Done_Glyph(ft_glyph);
          return nullptr;
        }
      }

      m_glyph.ft_glyph = ft_glyph;
      m_glyph.bearingX = face->glyph->metrics.horiBearingX / 64.0;
      m_glyph.bearingY = face->glyph->metrics.horiBearingY / 64.0;

      return &m_glyph;
    }

    void doneGlyph(Glyph* glyph) {
      ASSERT(glyph);
      FT_Done_Glyph(glyph->ft_glyph);
    }

  private:
    Glyph m_glyph;
  };

  class SimpleCache : public NoCache {
  public:
    ~SimpleCache() {
      invalidate();
    }

    void invalidate() {
      for (auto& it : m_glyphMap) {
        FT_Done_Glyph(it.second->ft_glyph);
        delete it.second;
      }

      m_glyphMap.clear();
    }

    Glyph* loadGlyph(FT_Face face, FT_UInt glyphIndex, bool antialias) {
      auto it = m_glyphMap.find(glyphIndex);
      if (it != m_glyphMap.end())
        return it->second;

      Glyph* glyph = NoCache::loadGlyph(face, glyphIndex, antialias);
      if (!glyph)
        return nullptr;

      FT_Glyph new_ft_glyph = nullptr;
      FT_Glyph_Copy(glyph->ft_glyph, &new_ft_glyph);
      if (!new_ft_glyph)
        return nullptr;

      Glyph* newGlyph = new Glyph(*glyph);
      newGlyph->ft_glyph = new_ft_glyph;

      m_glyphMap[glyphIndex] = newGlyph;
      FT_Done_Glyph(glyph->ft_glyph);

      return newGlyph;
    }

    void doneGlyph(Glyph* glyph) {
      // Do nothing
    }

  private:
    std::map<FT_UInt, Glyph*> m_glyphMap;
  };

  typedef FaceFT<SimpleCache> Face;

} // namespace ft
