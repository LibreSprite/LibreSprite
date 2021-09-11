// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_surface.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/sdl2/sdl2_surface.h"

#include "base/string.h"
#include "gfx/point.h"
#include "gfx/rect.h"

#include <iostream>
#include <SDL2/SDL.h>

namespace {

void checked_mode(int offset)
{
  // static BITMAP* pattern = NULL;
  // int x, y, fg, bg;

  // if (offset < 0) {
  //   if (pattern) {
  //     destroy_bitmap(pattern);
  //     pattern = NULL;
  //   }
  //   drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);
  //   return;
  // }

  // if (!pattern)
  //   pattern = create_bitmap(8, 8);

  // bg = makecol(0, 0, 0);
  // fg = makecol(255, 255, 255);
  // offset = 7 - (offset & 7);

  // clear_bitmap(pattern);

  // for (y=0; y<8; y++)
  //   for (x=0; x<8; x++)
  //     putpixel(pattern, x, y, ((x+y+offset)&7) < 4 ? fg: bg);

  // drawing_mode(DRAW_MODE_COPY_PATTERN, pattern, 0, 0);
}

}

namespace she {

namespace sdl {
    extern SDL_Surface* screen;
}

inline int to_sdl(SDL_PixelFormat* format, gfx::Color color)
{
    return SDL_MapRGBA(format, gfx::getr(color), gfx::getg(color), gfx::getb(color), gfx::geta(color));
}

inline gfx::Color from_sdl(SDL_PixelFormat *format, int color)
{
  return gfx::rgba(
      (color & format->Rmask) >> format->Rshift << format->Rloss,
      (color & format->Gmask) >> format->Gshift << format->Gloss,
      (color & format->Bmask) >> format->Bshift << format->Bloss,
      (color & format->Amask) >> format->Ashift << format->Aloss
  );
}

SDL2Surface::SDL2Surface(SDL_Surface* bmp, DestroyFlag destroy)
  : m_bmp(bmp)
  , m_destroy(destroy)
  , m_lock(0)
{
    SDL_SetSurfaceBlendMode(m_bmp, SDL_BLENDMODE_BLEND);
}

SDL2Surface::SDL2Surface(int width, int height, DestroyFlag destroy)
  : m_bmp(SDL_CreateRGBSurface(0, width, height, 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000))
  , m_destroy(destroy)
  , m_lock(0)
{
    if (!m_bmp) {
        std::cout << "Failed to create surface" << std::endl;
    } else {
        SDL_SetSurfaceBlendMode(m_bmp, SDL_BLENDMODE_BLEND);
    }
}

SDL2Surface::SDL2Surface(int width, int height, int bpp, DestroyFlag destroy)
  : m_bmp(SDL_CreateRGBSurface(0, width, height, bpp, 0xFF, 0xFF00, 0xFF0000, bpp == 32 ? 0xFF000000 : 0))
  , m_destroy(destroy)
  , m_lock(0)
{
    if (!m_bmp) {
        std::cout << "Failed to create surface" << std::endl;
    } else {
        SDL_SetSurfaceBlendMode(m_bmp, SDL_BLENDMODE_BLEND);
    }
}

SDL2Surface::~SDL2Surface()
{
  ASSERT(m_lock == 0);
  if (m_destroy & DestroyHandle) {
    if (m_bmp)
      SDL_FreeSurface(m_bmp);
  }
}

// Surface implementation

void SDL2Surface::dispose()
{
  if (m_destroy & DeleteThis)
    delete this;
}

int SDL2Surface::width() const
{
  return m_bmp->w;
}

int SDL2Surface::height() const
{
  return m_bmp->h;
}

bool SDL2Surface::isDirectToScreen() const
{
  return m_bmp == she::sdl::screen;
}

gfx::Rect SDL2Surface::getClipBounds()
{
    SDL_Rect rect;
    SDL_GetClipRect(m_bmp, &rect);
    return {rect.x, rect.y, rect.w, rect.h};
}

void SDL2Surface::setClipBounds(const gfx::Rect& rc)
{
    SDL_Rect rect{rc.x, rc.y, rc.w, rc.h};
    SDL_SetClipRect(m_bmp, &rect);
}

bool SDL2Surface::intersectClipRect(const gfx::Rect& rc)
{
    SDL_Rect rect = m_bmp->clip_rect;
    // SDL_GetClipRect(m_bmp, &rect);

    auto x = std::max(rect.x, rc.x);
    auto y = std::max(rect.y, rc.y);
    auto x2 = std::min(rect.x + rect.w, rc.x + rc.w);
    auto y2 = std::min(rect.y + rect.h, rc.y + rc.h);

    rect.x = x;
    rect.y = y;
    rect.w = x2 - x;
    rect.h = y2 - y;
    if (rect.x < 0) rect.x = 0;
    else if (rect.x >= m_bmp->w) rect.x = m_bmp->w - 1;
    if (rect.y < 0) rect.y = 0;
    else if (rect.y >= m_bmp->h) rect.y = m_bmp->h - 1;
    if (rect.w < 0) rect.w = 0;
    else if(rect.w + rect.x >= m_bmp->w) rect.w = m_bmp->w - rect.x - 1;
    if (rect.h < 0) rect.h = 0;
    else if(rect.h + rect.y >= m_bmp->h) rect.h = m_bmp->h - rect.y - 1;

    SDL_SetClipRect(m_bmp, &rect);

    return (x < x2) && (y < y2);
}

void SDL2Surface::lock()
{
  ASSERT(m_lock >= 0);
  if (m_lock++ == 0) {
      if (SDL_MUSTLOCK(m_bmp)) {
          SDL_LockSurface(m_bmp);
      }
  }
}

void SDL2Surface::unlock()
{
  ASSERT(m_lock > 0);
  if (--m_lock == 0) {
      if (SDL_MUSTLOCK(m_bmp)) {
          SDL_UnlockSurface(m_bmp);
      }
  }
}

void SDL2Surface::setDrawMode(DrawMode mode, int param)
{
  switch (mode) {
  case DrawMode::Solid: std::cout << "Solid" << std::endl; break;// checked_mode(-1); break;
  case DrawMode::Xor: std::cout << "Xor" << std::endl; break;// xor_mode(TRUE); break;
  case DrawMode::Checked: std::cout << "Checked" << std::endl; break;// checked_mode(param); break;
  }
}

void SDL2Surface::applyScale(int scale)
{
  if (scale < 2)
    return;

  SDL_Surface* scaled = SDL_CreateRGBSurfaceWithFormat(
      0,
      m_bmp->w * scale,
      m_bmp->h * scale,
      m_bmp->format->BitsPerPixel,
      m_bmp->format->format);

  SDL_Rect drect{0, 0, scaled->w, scaled->h};
  SDL_BlitScaled(m_bmp, nullptr, scaled, &drect);

  if (m_destroy & DestroyHandle)
    SDL_FreeSurface(m_bmp);

  m_bmp = scaled;
  m_destroy = DestroyHandle;
}

void* SDL2Surface::nativeHandle()
{
    return reinterpret_cast<void*>(m_bmp);
}

void SDL2Surface::clear()
{
    SDL_FillRect(m_bmp, nullptr, 0);
  // clear_to_color(m_bmp, 0);
}

uint8_t* SDL2Surface::getData(int x, int y) const
{
    return reinterpret_cast<uint8_t*>(m_bmp->pixels) + y * m_bmp->pitch + x * m_bmp->format->BytesPerPixel;
}

void SDL2Surface::getFormat(SurfaceFormatData* formatData) const
{
  formatData->format = kRgbaSurfaceFormat;
  formatData->bitsPerPixel = m_bmp->format->BitsPerPixel;
  formatData->redShift   = m_bmp->format->Rshift;
  formatData->greenShift = m_bmp->format->Gshift;
  formatData->blueShift  = m_bmp->format->Bshift;
  formatData->alphaShift = m_bmp->format->Ashift;
  formatData->redMask    = m_bmp->format->Rmask;
  formatData->greenMask  = m_bmp->format->Gmask;
  formatData->blueMask   = m_bmp->format->Bmask;
  formatData->alphaMask  = m_bmp->format->Amask;
}

gfx::Color SDL2Surface::getPixel(int x, int y) const
{
    int data = 0;
    if (m_bmp->format->BytesPerPixel == 4)
        data = *reinterpret_cast<uint32_t*>(getData(x, y));
    else if (m_bmp->format->BytesPerPixel == 2)
        data = *reinterpret_cast<uint16_t*>(getData(x, y));
    else if (m_bmp->format->BytesPerPixel == 1)
        data = *reinterpret_cast<uint8_t*>(getData(x, y));
    return from_sdl(m_bmp->format, data);
}

void SDL2Surface::putPixel(gfx::Color color, int x, int y)
{
  // putpixel(m_bmp, x, y, to_sdl(bitmap_color_depth(m_bmp), color));
  auto clip = m_bmp->clip_rect;
  if (x < clip.x || x >= clip.x + clip.w || y < clip.y || y >= clip.y + clip.h)
      return;
  int sdlColor = to_sdl(m_bmp->format, color);
  if (m_bmp->format->BytesPerPixel == 4)
      *reinterpret_cast<uint32_t*>(getData(x, y)) = sdlColor;
  else if (m_bmp->format->BytesPerPixel == 2)
      *reinterpret_cast<uint16_t*>(getData(x, y)) = sdlColor;
  else if (m_bmp->format->BytesPerPixel == 1)
      *reinterpret_cast<uint8_t*>(getData(x, y)) = sdlColor;
}

void SDL2Surface::drawHLine(gfx::Color color, int x, int y, int w)
{
    // if (gfx::geta(color) < 255) {
    //   set_trans_blender(0, 0, 0, gfx::geta(color));
    //   drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
    // }

    // hline(m_bmp, x, y, x+w-1, to_sdl(bitmap_color_depth(m_bmp), color));
    // solid_mode();
    SDL_Rect clip;
    SDL_GetClipRect(m_bmp, &clip);

    if (x < clip.x) {
        w += x - clip.x;
        x = clip.x;
    }

    if (x + w >= clip.x + clip.w) {
        w = (clip.x + clip.w) - x - 1;
    }

    if (w <= 0 || y < clip.y || y >= (clip.y + clip.h)) {
        return;
    }

    int sdlColor = to_sdl(m_bmp->format, color);
    auto data = getData(x, y);
    if (m_bmp->format->BytesPerPixel == 4) {
        for(; w--; data += 4)
            *reinterpret_cast<uint32_t*>(data) = sdlColor;
    } else if (m_bmp->format->BytesPerPixel == 2) {
        for(; w--; data += 2)
            *reinterpret_cast<uint16_t*>(data) = sdlColor;
    } else if (m_bmp->format->BytesPerPixel == 1) {
        for(; w--; data += 1)
            *reinterpret_cast<uint8_t*>(data) = sdlColor;
    }
}

void SDL2Surface::drawVLine(gfx::Color color, int x, int y, int h)
{
  // if (gfx::geta(color) < 255) {
  //   set_trans_blender(0, 0, 0, gfx::geta(color));
  //   drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
  // }

  // vline(m_bmp, x, y, y+h-1, to_sdl(bitmap_color_depth(m_bmp), color));
  // solid_mode();

    SDL_Rect clip;
    SDL_GetClipRect(m_bmp, &clip);

    if (y < clip.y) {
        h += y - clip.y;
        y = clip.y;
    }
    if (y + h >= clip.y + clip.h) {
        h = (clip.y + clip.h) - y - 1;
    }
    if (h <= 0 || x < clip.x || x >= clip.x + clip.w) {
        return;
    }
    int sdlColor = to_sdl(m_bmp->format, color);
    auto data = getData(x, y);
    auto stride = m_bmp->pitch;
    if (m_bmp->format->BytesPerPixel == 4) {
        for(; h--; data += stride)
            *reinterpret_cast<uint32_t*>(data) = sdlColor;
    } else if (m_bmp->format->BytesPerPixel == 2) {
        for(; h--; data += stride)
            *reinterpret_cast<uint16_t*>(data) = sdlColor;
    } else if (m_bmp->format->BytesPerPixel == 1) {
        for(; h--; data += stride)
            *reinterpret_cast<uint8_t*>(data) = sdlColor;
    }
}

void SDL2Surface::drawLine(gfx::Color color, const gfx::Point& a, const gfx::Point& b)
{
    if (a.x == b.x) {
        int y = a.y;
        int h = b.y - a.y;
        if (h < 0) {
            y = b.y;
            h = -h;
        }
        drawVLine(color, a.x, y, h);
        return;
    }

    if (a.y == b.y) {
        int x = a.x;
        int w = b.x - a.x;
        if (w < 0) {
            x = b.x;
            w = -w;
        }
        drawHLine(color, x, a.y, w);
        return;
    }

    printf("Unsupported line: %d,%d -> %d,%d\n", a.x, a.y, b.x, b.y);

  // if (gfx::geta(color) < 255) {
  //   set_trans_blender(0, 0, 0, gfx::geta(color));
  //   drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
  // }

  // line(m_bmp, a.x, a.y, b.x, b.y, to_sdl(bitmap_color_depth(m_bmp), color));
  // solid_mode();
}

void SDL2Surface::drawRect(gfx::Color color, const gfx::Rect& rc)
{
  // if (gfx::geta(color) < 255) {
  //   set_trans_blender(0, 0, 0, gfx::geta(color));
  //   drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
  // }

  // rect(m_bmp, rc.x, rc.y, rc.x+rc.w-1, rc.y+rc.h-1, to_sdl(bitmap_color_depth(m_bmp), color));
  // solid_mode();
    drawHLine(color, rc.x, rc.y, rc.w - 1);
    drawHLine(color, rc.x, rc.y + rc.h - 1, rc.w - 1);
    drawVLine(color, rc.x, rc.y, rc.h - 1);
    drawVLine(color, rc.x + rc.w - 1, rc.y, rc.h);
}

void SDL2Surface::fillRect(gfx::Color color, const gfx::Rect& rc)
{
  SDL_Rect rect;
  rect.x = rc.x;
  rect.y = rc.y;
  rect.w = rc.w;
  rect.h = rc.h;
  SDL_FillRect(m_bmp, &rect, to_sdl(m_bmp->format, color));
  // if (gfx::geta(color) < 255) {
  //   set_trans_blender(0, 0, 0, gfx::geta(color));
  //   drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
  // }

  // rectfill(m_bmp, rc.x, rc.y, rc.x+rc.w-1, rc.y+rc.h-1, to_sdl(bitmap_color_depth(m_bmp), color));
  // solid_mode();
}

void SDL2Surface::blitTo(Surface* dest, int srcx, int srcy, int dstx, int dsty, int width, int height) const
{
  ASSERT(m_bmp);
  ASSERT(dest);
  ASSERT(static_cast<SDL2Surface*>(dest)->m_bmp);

  SDL_Rect srect{srcx, srcy, width, height},
           drect{dstx, dsty, width, height};

  auto srcbmp = static_cast<const SDL2Surface*>(dest)->m_bmp;
  auto slocked = srcbmp->locked;
  auto dlocked = m_bmp->locked;
  if (slocked) SDL_UnlockSurface(srcbmp);
  if (dlocked) SDL_UnlockSurface(m_bmp);
  SDL_BlitSurface(m_bmp, &srect, srcbmp, &drect);
  if (slocked) SDL_LockSurface(srcbmp);
  if (dlocked) SDL_LockSurface(m_bmp);
}

void SDL2Surface::scrollTo(const gfx::Rect& rc, int dx, int dy)
{
    int w = width();
    int h = height();
    gfx::Clip clip(rc.x+dx, rc.y+dy, rc);
    if (!clip.clip(w, h, w, h))
      return;

    int bytesPerPixel = m_bmp->format->BytesPerPixel;
    int rowBytes = (int)m_bmp->pitch;
    int rowDelta;

    if (dy > 0) {
      clip.src.y += clip.size.h-1;
      clip.dst.y += clip.size.h-1;
      rowDelta = -rowBytes;
    }
    else
      rowDelta = rowBytes;

    char* dst = (char*)m_bmp->pixels;
    const char* src = dst;
    dst += rowBytes*clip.dst.y + bytesPerPixel*clip.dst.x;
    src += rowBytes*clip.src.y + bytesPerPixel*clip.src.x;
    w = bytesPerPixel*clip.size.w;
    h = clip.size.h;

    while (--h >= 0) {
      memmove(dst, src, w);
      dst += rowDelta;
      src += rowDelta;
    }
}

void SDL2Surface::drawSurface(const Surface* src, int dstx, int dsty)
{
    SDL_Rect rect{dstx, dsty, src->width(), src->height()};
    auto srcbmp = static_cast<const SDL2Surface*>(src)->m_bmp;
    auto slocked = srcbmp->locked;
    auto dlocked = m_bmp->locked;
    if (slocked) SDL_UnlockSurface(srcbmp);
    if (dlocked) SDL_UnlockSurface(m_bmp);
    SDL_BlitSurface(srcbmp, nullptr, m_bmp, &rect);
    if (slocked) SDL_LockSurface(srcbmp);
    if (dlocked) SDL_LockSurface(m_bmp);
}

void SDL2Surface::drawRgbaSurface(const Surface* src, int dstx, int dsty)
{
    SDL_Rect rect{dstx, dsty, src->width(), src->height()};
    auto srcbmp = static_cast<const SDL2Surface*>(src)->m_bmp;
    auto slocked = srcbmp->locked;
    auto dlocked = m_bmp->locked;

    SDL_SetSurfaceBlendMode(m_bmp, SDL_BLENDMODE_BLEND);
    if (slocked) SDL_UnlockSurface(srcbmp);
    if (dlocked) SDL_UnlockSurface(m_bmp);
    SDL_BlitSurface(srcbmp, nullptr, m_bmp, &rect);
    if (slocked) SDL_LockSurface(srcbmp);
    if (dlocked) SDL_LockSurface(m_bmp);
}

} // namespace she
