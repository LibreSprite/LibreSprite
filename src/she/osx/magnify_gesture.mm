// SHE library - macOS Magnify Gesture Support
// Copyright (C) 2024 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef __APPLE__

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#else
#include <SDL.h>
#include <SDL_syswm.h>
#endif

#include "she/event.h"
#include "she/event_queue.h"

// Store the original method implementation
static IMP s_originalMagnifyWithEvent = nullptr;

// Our custom magnifyWithEvent: handler
static void custom_magnifyWithEvent(id self, SEL _cmd, NSEvent* event) {
    // Create and queue a TouchMagnify event
    she::Event ev;
    ev.setType(she::Event::TouchMagnify);
    ev.setMagnification([event magnification]);

    // Get mouse position from the event's window
    NSWindow* window = [event window];
    if (window) {
        NSView* contentView = [window contentView];
        if (contentView) {
            NSPoint point = [contentView convertPoint:[event locationInWindow] fromView:nil];
            NSRect bounds = [contentView bounds];

            // Convert to she coordinates (origin at top-left)
            int x = static_cast<int>(point.x);
            int y = static_cast<int>(bounds.size.height - point.y);
            ev.setPosition(gfx::Point(x, y));
        }
    }

    ev.setPointerType(she::PointerType::Multitouch);

    she::queue_event(ev);

    // Call original implementation if it exists
    if (s_originalMagnifyWithEvent) {
        ((void (*)(id, SEL, NSEvent*))s_originalMagnifyWithEvent)(self, _cmd, event);
    }
}

namespace osx_magnify {

bool init(SDL_Window* window) {
    if (!window) {
        return false;
    }

    // Get the NSWindow from SDL
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(window, &wmInfo)) {
        return false;
    }

    if (wmInfo.subsystem != SDL_SYSWM_COCOA) {
        return false;
    }

    NSWindow* nsWindow = wmInfo.info.cocoa.window;
    if (!nsWindow) {
        return false;
    }

    NSView* contentView = [nsWindow contentView];
    if (!contentView) {
        return false;
    }

    // Get the class of SDL's content view
    Class viewClass = [contentView class];

    // Check if magnifyWithEvent: is already implemented
    SEL magnifySel = @selector(magnifyWithEvent:);
    Method existingMethod = class_getInstanceMethod(viewClass, magnifySel);

    if (existingMethod) {
        // Store the original implementation and replace it
        s_originalMagnifyWithEvent = method_setImplementation(existingMethod, (IMP)custom_magnifyWithEvent);
    } else {
        // Add the method to the class
        class_addMethod(viewClass, magnifySel, (IMP)custom_magnifyWithEvent, "v@:@");
    }

    // Enable gesture events for the view
    if (@available(macOS 10.12.2, *)) {
        contentView.allowedTouchTypes = NSTouchTypeMaskDirect | NSTouchTypeMaskIndirect;
    } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        [contentView setAcceptsTouchEvents:YES];
#pragma clang diagnostic pop
    }

    return true;
}

} // namespace osx_magnify

#endif // __APPLE__
