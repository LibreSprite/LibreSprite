// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "base/disable_copying.h"
#include "base/unique_ptr.h"
#include "she/skia/skia_surface.h"
#include "she/win/window.h"

#if SK_SUPPORT_GPU
  #include "gl/GrGLInterface.h"
  #include "she/gl/gl_context.h"
#endif

namespace she {

class EventQueue;
class SkiaDisplay;

class SkiaWindow : public WinWindow<SkiaWindow> {
public:
  enum class Backend { NONE, GL, ANGLE };

  SkiaWindow(EventQueue* queue, SkiaDisplay* display,
             int width, int height, int scale);
  ~SkiaWindow();

  void queueEventImpl(Event& ev);
  void paintImpl(HDC hdc);
  void resizeImpl(const gfx::Size& size);

private:
  void paintHDC(HDC dc);

#if SK_SUPPORT_GPU
#if SK_ANGLE
  bool attachANGLE();
#endif // SK_ANGLE
  bool attachGL();
  void detachGL();
  void createRenderTarget(const gfx::Size& size);
#endif // SK_SUPPORT_GPU

  EventQueue* m_queue;
  SkiaDisplay* m_display;
  Backend m_backend;
#if SK_SUPPORT_GPU
  base::UniquePtr<GLContext> m_glCtx;
  SkAutoTUnref<const GrGLInterface> m_glInterfaces;
  sk_sp<GrContext> m_grCtx;
  sk_sp<GrRenderTarget> m_grRenderTarget;
  sk_sp<SkSurface> m_skSurfaceDirect;
  sk_sp<SkSurface> m_skSurface;
  int m_sampleCount;
  int m_stencilBits;
  gfx::Size m_lastSize;
#endif // SK_SUPPORT_GPU

  DISABLE_COPYING(SkiaWindow);
};

} // namespace she
