// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/unique_ptr.h"
#include "doc/cel.h"
#include "doc/frame.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "gfx/rect.h"
#include "render/render.h"

namespace app {

using namespace doc;

static bool has_cels(const Layer* layer, frame_t frame);

LayerImage* create_flatten_layer_copy(Sprite* dstSprite, const Layer* srcLayer,
                                      const gfx::Rect& bounds,
                                      frame_t frmin, frame_t frmax)
{
  base::UniquePtr<LayerImage> flatLayer(new LayerImage(dstSprite));
  render::Render render;

  for (frame_t frame=frmin; frame<=frmax; ++frame) {
    // Does this frame have cels to render?
    if (has_cels(srcLayer, frame)) {
      // Create a new image to render each frame.
      ImageRef image(Image::create(flatLayer->sprite()->pixelFormat(), bounds.w, bounds.h));

      // Create the new cel for the output layer.
      base::UniquePtr<Cel> cel(new Cel(frame, image));
      cel->setPosition(bounds.x, bounds.y);

      // Render this frame.
      render.renderLayer(image.get(), srcLayer, frame,
        gfx::Clip(0, 0, bounds));

      // Add the cel (and release the base::UniquePtr).
      flatLayer->addCel(cel);
      cel.release();
    }
  }

  return flatLayer.release();
}

// Returns true if the "layer" or its children have any cel to render
// in the given "frame".
static bool has_cels(const Layer* layer, frame_t frame)
{
  if (!layer->isVisible())
    return false;

  switch (layer->type()) {

    case ObjectType::LayerImage:
      return (layer->cel(frame) ? true: false);

    case ObjectType::LayerFolder: {
      LayerConstIterator it = static_cast<const LayerFolder*>(layer)->getLayerBegin();
      LayerConstIterator end = static_cast<const LayerFolder*>(layer)->getLayerEnd();

      for (; it != end; ++it) {
        if (has_cels(*it, frame))
          return true;
      }
      break;
    }

  }

  return false;
}

} // namespace app
