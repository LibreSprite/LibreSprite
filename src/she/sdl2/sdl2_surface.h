// SHE library
// Copyright (C) 2021 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SDL2_SURFACE_H
#define SHE_SDL2_SURFACE_H
#pragma once

#include "she/surface.h"
#include "she/common/generic_surface.h"
#include "she/sdl2/sdl2_display.h"

struct SDL_Surface;

namespace she {

  class SDL2Surface : public GenericDrawTextSurface<GenericDrawColoredRgbaSurface<Surface> > {
  public:
    enum DestroyFlag {
      None = 0,
      DeleteThis = 1,
      DestroyHandle = 2,
      DeleteAndDestroy = DeleteThis | DestroyHandle,
    };

    SDL2Surface(SDL_Surface* bmp, DestroyFlag destroy);
    SDL2Surface(int width, int height, DestroyFlag destroy);
    SDL2Surface(int width, int height, int bpp, DestroyFlag destroy);
    ~SDL2Surface();

    // Surface implementation
    void dispose() override;
    int width() const override;
    int height() const override;
    bool isDirectToScreen() const override;
    gfx::Rect getClipBounds() override;
    void setClipBounds(const gfx::Rect& rc) override;
    bool intersectClipRect(const gfx::Rect& rc) override;
    void lock() override;
    void unlock() override;
    void setDrawMode(DrawMode mode, int param) override;
    void applyScale(int scale) override;
    void* nativeHandle() override;
    void clear() override;
    uint8_t* getData(int x, int y) const override;
    void getFormat(SurfaceFormatData* formatData) const override;
    gfx::Color getPixel(int x, int y) const override;
    void putPixel(gfx::Color color, int x, int y) override;
    void drawHLine(gfx::Color color, int x, int y, int w) override;
    void drawVLine(gfx::Color color, int x, int y, int h) override;
    void drawLine(gfx::Color color, const gfx::Point& a, const gfx::Point& b) override;
    void drawRect(gfx::Color color, const gfx::Rect& rc) override;
    void fillRect(gfx::Color color, const gfx::Rect& rc) override;
    void blitTo(Surface* dest, int srcx, int srcy, int dstx, int dsty, int width, int height) const override;
    void scrollTo(const gfx::Rect& rc, int dx, int dy) override;
    void drawSurface(const Surface* src, int dstx, int dsty) override;
    void drawRgbaSurface(const Surface* src, int dstx, int dsty) override;

    SDL_Texture* getTexture(SDL_Rect& rect);

    static inline unsigned int textureGen;

  private:
    unsigned int m_textureGen{};
    SDL_Texture* m_texture{};
    uint32_t m_textureFormat{};
    SDL_Surface* m_bmp{};
    DestroyFlag m_destroy;
    int m_lock;
  };

} // namespace she

#endif
