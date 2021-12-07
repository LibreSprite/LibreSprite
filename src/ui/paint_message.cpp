#include "message.h"
#include "manager.h"
#include "she/display.h"
#include "she/surface.h"

bool ui::PaintMessage::sendToWidget(Widget* widget) {
    bool done = false;
    if (widget->hasFlags(HIDDEN))
        return done;

    auto manager = widget->manager();
    auto display = manager->getDisplay();
    if (!display)
        return done;

    she::Surface* surface = display->getSurface();
    gfx::Rect oldClip = surface->getClipBounds();

    if (surface->intersectClipRect(m_rect)) {
#ifdef REPORT_EVENTS
        std::cout << " - clip("
                  << paintMsg->rect().x << ", "
                  << paintMsg->rect().y << ", "
                  << paintMsg->rect().w << ", "
                  << paintMsg->rect().h << ")"
                  << std::endl;
#endif

#ifdef DEBUG_PAINT_EVENTS
        {
            she::SurfaceLock lock(surface);
            surface->fillRect(gfx::rgba(0, 0, 255), paintMsg->rect());
        }
        display->flip(gfx::Rect(0, 0, display->width(), display->height()));
        std::this_thread::sleep_for(0.002);
#endif

        // Call the message handler
        done = widget->sendMessage(this);
        // Restore clip region for paint messages.
        surface->setClipBounds(oldClip);
    }

    // As this kPaintMessage's rectangle was updated, we can
    // remove it from "m_invalidRegion".
    manager->removeInvalidRegion(gfx::Region{m_rect});
    return done;
}
