// SHE library
// Copyright (C) 2021  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SDL2_DISPLAY_H
#define SHE_SDL2_DISPLAY_H
#pragma once

#include <SDL2/SDL_mouse.h>
#include "she/display.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Surface;
struct SDL_Texture;

namespace she {

    class SDL2Display : public Display {
    public:
        SDL2Display(int width, int height, int scale);
        ~SDL2Display();

        void dispose() override;
        int width() const override;
        int height() const override;
        void setWidth(int);
        void setHeight(int);
        int originalWidth() const override;
        int originalHeight() const override;
        void setOriginalWidth(int width);
        void setOriginalHeight(int height);
        int scale() const override;
        void setScale(int scale) override;
        void recreateSurface();
        Surface* getSurface() override;
        void flip(const gfx::Rect& bounds) override;
        void maximize() override;
        bool isMaximized() const override;
        bool isMinimized() const override;
        void setTitleBar(const std::string& title) override;
        NativeCursor nativeMouseCursor() override;
        bool setNativeMouseCursor(NativeCursor cursor) override;
        void setMousePosition(const gfx::Point& position) override;
        void captureMouse() override;
        void releaseMouse() override;
        std::string getLayout() override;
        void setLayout(const std::string& layout) override;
        void* nativeHandle() override;

        void present();
    private:
        SDL_Surface* m_nativeSurface;
        SDL_Window* m_window;
        SDL_Renderer* m_renderer;
        SDL_Cursor* m_cursor;
        Surface* m_surface;
        int m_scale;
        int m_width;
        int m_height;
        NativeCursor m_nativeCursor;
        int m_restoredWidth;
        int m_restoredHeight;
        bool m_dirty = true;
    };

    extern SDL2Display* unique_display;

} // namespace she

#endif
