// SHE library
// Copyright (C) 2021-2026 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/she.h"

#include "base/concurrent_queue.h"
#include "base/exception.h"
#include "base/string.h"
#include "she/sdl2/sdl2_display.h"
#include "she/sdl2/sdl2_surface.h"
#include "she/common/system.h"
#include "she/logger.h"

#undef HAVE_STDINT_H
#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#include <SDL2/SDL_hints.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_syswm.h>
#else
#include <SDL.h>
#include <SDL_hints.h>
#include <SDL_image.h>
#include <SDL_syswm.h>
#endif

#include <iostream>
#include <cassert>
#include <list>
#include <vector>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <chrono>
#include <cmath>
#include <map>
#include <optional>
#include <thread>

#define SDL_HINT_WINDOWS_DPI_AWARENESS "SDL_WINDOWS_DPI_AWARENESS"

float penPressure = 0;

namespace ui {
  float get_pen_pressure() {
    return penPressure;
  }
}

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
extern "C" {
    void onPointerEvent(int isPen, float pressure) {
	if (isPen) {
	    penPressure = pressure > 0 ? pressure : 0.0001;
	} else {
	    penPressure = 0;
	}
    }
}
extern bool cfginit();
#endif

static she::System* g_instance = nullptr;
static std::unordered_map<int, she::Event::MouseButton> mouseButtonMapping = {
  {SDL_BUTTON_LEFT, she::Event::LeftButton},
  {SDL_BUTTON_MIDDLE, she::Event::MiddleButton},
  {SDL_BUTTON_RIGHT, she::Event::RightButton}
};
static she::KeyScancode lastScancode;
static int lastScancodeSDL;
struct Modifier {
  const int sheModifier;
  int ascii;
  bool isPressed = false;
  Modifier(int sheModifier) : sheModifier(sheModifier) {}
};

static std::unordered_map<int, Modifier*> reverseKeyCodeMapping;

static std::unordered_map<SDL_Keycode, Modifier> keyCodeMapping = {
  {SDLK_UNKNOWN, she::kKeyNil},
  {SDL_Keycode(13), she::kKeyEnter},
  {SDLK_PERIOD, she::kKeyStop},
  {SDLK_a, she::kKeyA},
  {SDLK_b, she::kKeyB},
  {SDLK_c, she::kKeyC},
  {SDLK_d, she::kKeyD},
  {SDLK_e, she::kKeyE},
  {SDLK_f, she::kKeyF},
  {SDLK_g, she::kKeyG},
  {SDLK_h, she::kKeyH},
  {SDLK_i, she::kKeyI},
  {SDLK_j, she::kKeyJ},
  {SDLK_k, she::kKeyK},
  {SDLK_l, she::kKeyL},
  {SDLK_m, she::kKeyM},
  {SDLK_n, she::kKeyN},
  {SDLK_o, she::kKeyO},
  {SDLK_p, she::kKeyP},
  {SDLK_q, she::kKeyQ},
  {SDLK_r, she::kKeyR},
  {SDLK_s, she::kKeyS},
  {SDLK_t, she::kKeyT},
  {SDLK_u, she::kKeyU},
  {SDLK_v, she::kKeyV},
  {SDLK_w, she::kKeyW},
  {SDLK_x, she::kKeyX},
  {SDLK_y, she::kKeyY},
  {SDLK_z, she::kKeyZ},
  {SDLK_0, she::kKey0},
  {SDLK_1, she::kKey1},
  {SDLK_2, she::kKey2},
  {SDLK_3, she::kKey3},
  {SDLK_4, she::kKey4},
  {SDLK_5, she::kKey5},
  {SDLK_6, she::kKey6},
  {SDLK_7, she::kKey7},
  {SDLK_8, she::kKey8},
  {SDLK_9, she::kKey9},
  {SDLK_KP_0, she::kKey0Pad},
  {SDLK_KP_1, she::kKey1Pad},
  {SDLK_KP_2, she::kKey2Pad},
  {SDLK_KP_3, she::kKey3Pad},
  {SDLK_KP_4, she::kKey4Pad},
  {SDLK_KP_5, she::kKey5Pad},
  {SDLK_KP_6, she::kKey6Pad},
  {SDLK_KP_7, she::kKey7Pad},
  {SDLK_KP_8, she::kKey8Pad},
  {SDLK_KP_9, she::kKey9Pad},
  {SDLK_F1, she::kKeyF1},
  {SDLK_F2, she::kKeyF2},
  {SDLK_F3, she::kKeyF3},
  {SDLK_F4, she::kKeyF4},
  {SDLK_F5, she::kKeyF5},
  {SDLK_F6, she::kKeyF6},
  {SDLK_F7, she::kKeyF7},
  {SDLK_F8, she::kKeyF8},
  {SDLK_F9, she::kKeyF9},
  {SDLK_F10, she::kKeyF10},
  {SDLK_F11, she::kKeyF11},
  {SDLK_F12, she::kKeyF12},
  {SDLK_ESCAPE, she::kKeyEsc},
  {SDLK_QUOTE, she::kKeyTilde},
  {SDLK_MINUS, she::kKeyMinus},
  {SDLK_EQUALS, she::kKeyEquals},
  {SDLK_BACKSPACE, she::kKeyBackspace},
  {SDLK_TAB, she::kKeyTab},
  {SDLK_LEFTBRACKET, she::kKeyOpenbrace},
  {SDLK_RIGHTBRACKET, she::kKeyClosebrace},
  {SDLK_KP_ENTER, she::kKeyEnter},
  {SDLK_COLON, she::kKeyColon},
  {SDLK_QUOTE, she::kKeyQuote},
  {SDLK_BACKSLASH, she::kKeyBackslash},
  // {SDLK_BACKSLASH2, she::kKeyBackslash2},
  {SDLK_COMMA, she::kKeyComma},
  {SDLK_STOP, she::kKeyStop},
  {SDLK_SLASH, she::kKeySlash},
  {SDLK_SPACE, she::kKeySpace},
  {SDLK_INSERT, she::kKeyInsert},
  {SDLK_DELETE, she::kKeyDel},
  {SDLK_HOME, she::kKeyHome},
  {SDLK_END, she::kKeyEnd},
  {SDLK_PAGEUP, she::kKeyPageUp},
  {SDLK_PAGEDOWN, she::kKeyPageDown},
  {SDLK_LEFT, she::kKeyLeft},
  {SDLK_RIGHT, she::kKeyRight},
  {SDLK_UP, she::kKeyUp},
  {SDLK_DOWN, she::kKeyDown},
  {SDLK_KP_DIVIDE, she::kKeySlashPad},
  {SDLK_ASTERISK, she::kKeyAsterisk},
  {SDLK_KP_MINUS, she::kKeyMinusPad},
  {SDLK_KP_PLUS, she::kKeyPlusPad},
  // {SDLK_KP_DEL, she::kKeyDelPad},
  {SDLK_KP_PERIOD, she::kKeyDelPad},
  {SDLK_KP_ENTER, she::kKeyEnterPad},
  {SDLK_PRINTSCREEN, she::kKeyPrtscr},
  {SDLK_PAUSE, she::kKeyPause},
  // {SDLK_ABNTC1, she::kKeyAbntC1},
  // {SDLK_YEN, she::kKeyYen},
  // {SDLK_KANA, she::kKeyKana},
  // {SDLK_CONVERT, she::kKeyConvert},
  // {SDLK_NOCONVERT, she::kKeyNoconvert},
  {SDLK_AT, she::kKeyAt},
  // {SDLK_CIRCUMFLEX, she::kKeyCircumflex},
  // {SDLK_COLON2, she::kKeyColon2},
  // {SDLK_KANJI, she::kKeyKanji},
  {SDLK_KP_EQUALS, she::kKeyEqualsPad},
  {SDLK_BACKQUOTE, she::kKeyBackquote},
  {SDLK_SEMICOLON, she::kKeySemicolon},
  // {SDLK_COMMAND, she::kKeyCommand},
  // {SDLK_UNKNOWN1, she::kKeyUnknown1},
  // {SDLK_UNKNOWN2, she::kKeyUnknown2},
  // {SDLK_UNKNOWN3, she::kKeyUnknown3},
  // {SDLK_UNKNOWN4, she::kKeyUnknown4},
  // {SDLK_UNKNOWN5, she::kKeyUnknown5},
  // {SDLK_UNKNOWN6, she::kKeyUnknown6},
  // {SDLK_UNKNOWN7, she::kKeyUnknown7},
  // {SDLK_UNKNOWN8, she::kKeyUnknown8},
  {SDLK_LSHIFT, she::kKeyLShift},
  {SDLK_RSHIFT, she::kKeyRShift},
  {SDLK_LCTRL, she::kKeyLControl},
  {SDLK_RCTRL, she::kKeyRControl},
  {SDLK_LALT, she::kKeyAlt},
  {SDLK_RALT, she::kKeyAltGr},
  {SDL_Keycode(1073742051), she::kKeyLWin},
  // {SDLK_RWIN, she::kKeyRWin},
  {SDLK_MENU, she::kKeyMenu},
  {SDLK_SCROLLLOCK, she::kKeyScrLock},
  {SDLK_NUMLOCKCLEAR, she::kKeyNumLock},
  {SDLK_CAPSLOCK, she::kKeyCapsLock},
};

std::unordered_map<SDL_Keycode, Modifier> modifiers = {
  {SDLK_SPACE, she::kKeySpaceModifier},

  {SDLK_LALT, she::kKeyAltModifier},
  {SDLK_RALT, she::kKeyAltModifier},

  {SDLK_LCTRL, she::kKeyCtrlModifier},
  {SDLK_RCTRL, she::kKeyCtrlModifier},

  {SDLK_LGUI, she::kKeyCmdModifier},
  {SDLK_RGUI, she::kKeyCmdModifier},

  {SDLK_LSHIFT, she::kKeyShiftModifier},
  {SDLK_RSHIFT, she::kKeyShiftModifier}
};

she::KeyModifiers getSheModifiers() {
  int mod = 0;
  for (auto& entry : modifiers) {
    if (entry.second.isPressed)
      mod |= entry.second.sheModifier;
  }
  return (she::KeyModifiers) mod;
}

#ifdef __EMSCRIPTEN__
EM_JS(int, get_canvas_width, (), { return canvas.clientWidth; });
EM_JS(int, get_canvas_height, (), { return canvas.clientHeight; });

// Sends fingers and styluses to C++ (onTouchPointer) and leaves the mouse to SDL, and makes the
// hidden text box an on-screen keyboard types into (onTypedText, onTypedKey).
EM_JS(void, setup_touch_and_typing, (), {
  const canvas = Module.canvas;
  canvas.style.touchAction = 'none';  // the page must not scroll or zoom under the fingers

  const mouse = Module.cwrap('onPointerEvent', '', ['number', 'number']);
  const touch = Module.cwrap('onTouchPointer', '', ['number', 'number', 'number', 'number', 'number', 'number']);
  const types = {pointerdown: 0, pointermove: 1, pointerup: 2, pointercancel: 3};
  const listener = event => {
    if (event.pointerType == 'mouse')
      return mouse(0, 0);
    event.preventDefault();  // and the browser makes no mouse events of its own from it
    const r = canvas.getBoundingClientRect();
    touch(types[event.type], event.pointerId, event.pointerType == 'pen' ? 1 : 0,
          event.clientX - r.left, event.clientY - r.top, event.pressure);
  };
  for (const type in types)
    canvas.addEventListener(type, listener);

  const typing = document.createElement('input');
  typing.id = 'typing';
  const attributes = {autocapitalize: 'off', autocorrect: 'off', autocomplete: 'off', spellcheck: 'false',
                      enterkeyhint: 'done', 'aria-label': 'Typing for LibreSprite'};
  for (const name in attributes)
    typing.setAttribute(name, attributes[name]);
  typing.style.cssText = 'position:fixed;left:0;top:0;width:1px;height:1px;opacity:0;border:0;padding:0;margin:0;font-size:16px;pointer-events:none';
  document.body.appendChild(typing);

  const text = Module.cwrap('onTypedText', '', ['string']);
  const key = Module.cwrap('onTypedKey', '', ['number', 'number', 'number', 'number', 'number']);
  // The box keeps a space, so a backspace is still reported when it looks empty.
  const reset = () => {
    typing.value = ' ';
    typing.setSelectionRange(1, 1);
  };
  typing.addEventListener('focus', reset);
  typing.addEventListener('input', event => {
    if (event.inputType && event.inputType.startsWith('delete')) {
      key(1, 8, 0, 0, 0);
      key(0, 8, 0, 0, 0);
    }
    else if (event.data)
      text(event.data);
    reset();
  });
  // Tab, Enter, Escape, End, Home, the arrows and Delete, and shortcuts from an attached
  // keyboard. Letters come from the input events above; modifiers alone are never sent, so
  // none can stay stuck.
  const named = [9, 13, 27, 35, 36, 37, 38, 39, 40, 46];
  for (const type of ['keydown', 'keyup'])
    typing.addEventListener(type, event => {
      event.stopPropagation();  // SDL only sees keys typed elsewhere
      const shortcut = event.ctrlKey || event.metaKey || event.altKey;
      if (!named.includes(event.keyCode) && !(shortcut && event.keyCode >= 48 && event.keyCode <= 90))
        return;
      event.preventDefault();
      key(type == 'keydown' ? 1 : 0, event.keyCode, event.ctrlKey || event.metaKey ? 1 : 0,
          event.shiftKey ? 1 : 0, event.altKey ? 1 : 0);
    });
  typing.addEventListener('keypress', event => event.stopPropagation());

  // Tablets open their keyboard only for a focus made during a tap, so a tap inside the focused
  // text box (set_input_rect says where it is) brings it up.
  let textRect = null, putAway = 0;
  Module.setTextRect = (x, y, w, h) => {
    clearTimeout(putAway);
    if (w <= 0 || h <= 0) {
      textRect = null;
      // Focus often goes straight from one text box to another (a dialog opening), and a
      // keyboard put away then would not come back without a tap. Only put it away when no
      // other text box takes the focus.
      putAway = setTimeout(() => {
        if (!textRect && document.activeElement == typing)
          typing.blur();
      }, 100);
      return;
    }
    textRect = {x: x, y: y, w: w, h: h};
    const r = canvas.getBoundingClientRect();
    Object.assign(typing.style, {left: (r.left + x) + 'px', top: (r.top + y) + 'px', width: w + 'px', height: h + 'px'});
  };
  for (const type of ['touchstart', 'touchmove', 'touchend', 'touchcancel'])
    canvas.addEventListener(type, event => {
      event.preventDefault();  // no mouse events or double-tap zoom from the browser
      const t = event.changedTouches[0];
      if (type != 'touchend' || event.touches.length || !t || !textRect)
        return;
      const r = canvas.getBoundingClientRect();
      const x = t.clientX - r.left, y = t.clientY - r.top;
      if (x >= textRect.x && y >= textRect.y && x < textRect.x + textRect.w && y < textRect.y + textRect.h)
        typing.focus();
    }, {passive: false});

  // A Keyboard button, if the page has one, brings the keyboard up or hides it. The tap on the
  // button may close the keyboard first, so note whether it was up when the finger landed.
  const button = document.getElementById('keyboard');
  if (button) {
    let wasUp = false;
    button.addEventListener('pointerdown', () => { wasUp = document.activeElement == typing; });
    button.addEventListener('click', () => wasUp ? typing.blur() : typing.focus());
  }
});
static int oldWidth, oldHeight;

static void addEventListener(const std::string& name, void (*function)(void*), void* data = nullptr) {
  EM_ASM({
    canvas.addEventListener(UTF8ToString($0), (event) => {
      window.event = event;
      dynCall('vi', $1, [$2]);
    });
  }, name.c_str(), function, data);
}

static void cancelEvent(void*) {
  EM_ASM({
    event.stopPropagation();
    event.preventDefault();
  });
}

static bool wrapped;
static void patchEventListeners() {
  if (wrapped)
    return;
  wrapped = EM_ASM_INT({
    let handle = 0;
    JSEvents.eventHandlers.forEach(handler => {
	if (handler.eventTypeString == 'keydown' || handler.eventTypeString == 'keyup') {
          handle = handler.callbackfunc;
	  let tmp = getWasmTableEntry(handle);
	  function wrapper(...args) {
	    let ret = tmp(...args);
	    let keyCode = GROWABLE_HEAP_I32()[(args[1] >> 2) + 9];
	    return ret && keyCode != 86;
	  }
	  if (tmp.name != wrapper.name)
	    wasmTableMirror[handle] = wrapper;
	}
    });
    return !!handle;
  });
}

#endif

static std::deque<she::Event> keybuffer;
static bool display_has_mouse = false;

#ifdef __EMSCRIPTEN__
// Touch, stylus and on-screen keyboards on the web.
//
// A tablet's browser reports fingers and styluses as pointer events. SDL would turn every
// finger into a mouse, so a finger always draws and there is no way to zoom or scroll by
// touch. Instead they come here (onTouchPointer) and become the events the desktop interface
// already understands:
//   stylus       draws, with its pressure, and hovers;
//   one finger   a tap clicks. It draws only until a stylus has been seen, so a resting hand
//                leaves no marks;
//   two fingers  pinch to zoom (TouchMagnify) or slide to scroll (a precise wheel), whichever
//                they start doing; a quick tap is Ctrl+Z;
//   three        a quick tap is Ctrl+Y.
// What's typed on an on-screen keyboard arrives in a hidden text box and comes here as text
// (onTypedText) and keys (onTypedKey).
namespace {
  enum { kPointerDown, kPointerMove, kPointerUp, kPointerCancel };
  constexpr double kTapMs = 300;        // a tap is shorter than this...
  constexpr double kTapPx = 12;         // ...and moves less than this (CSS pixels)
  constexpr double kHoldMs = 120;       // a lone finger draws only once it has been down this long
  constexpr double kDoubleTapMs = 300;  // two taps this close are a double click

  struct Finger { double x, y; };
  std::map<int, Finger> fingers;
  bool stylusSeen = false;
  bool penDown = false;

  // From the first finger down to the last one up.
  struct Gesture {
    double t0, x0, y0, lx, ly;  // when and where it started; where the first finger is now
    int max = 1;                // the most fingers down at once
    bool moved = false, drawing = false, drew = false, done = false;
    enum { Undecided, Zoom, Scroll } mode = Undecided;
    double cx, cy, d0;          // two fingers: their centre and spread when the second landed
    double dRef;                // the spread at the last zoom step
    double px, py;              // the centre at the last scroll step
    double sx = 0, sy = 0;      // scroll not sent yet, in CSS pixels
  };
  std::optional<Gesture> g;
  double lastTap = -1e9, lastTapX, lastTapY;
  unsigned pointerEvents = 0;  // touch and stylus events so far

  int displayScale() {
    return she::unique_display ? she::unique_display->scale() : 1;
  }

  void post(she::Event::Type type, double x, double y,
            she::PointerType pointer = she::PointerType::Mouse, float pressure = 0,
            she::Event::MouseButton button = she::Event::NoneButton) {
    // After a leave, the interface takes mouse events again once the pointer has entered.
    if (type == she::Event::MouseLeave)
      display_has_mouse = false;
    else if (!display_has_mouse) {
      display_has_mouse = true;
      post(she::Event::MouseEnter, x, y, pointer);
    }
    she::Event ev;
    ev.setType(type);
    ev.setPosition(gfx::Point(int(x) / displayScale(), int(y) / displayScale()));
    ev.setModifiers(she::kKeyNoneModifier);
    ev.setPointerType(pointer);
    ev.setPressure(pressure);
    ev.setButton(button);
    she::EventQueue::instance()->queueEvent(ev);
  }

  void key(she::Event::Type type, she::KeyScancode scancode, she::KeyModifiers modifiers, int unicodeChar = 0) {
    she::Event ev;
    ev.setType(type);
    ev.setScancode(scancode);
    ev.setModifiers(modifiers);
    ev.setUnicodeChar(unicodeChar);
    she::EventQueue::instance()->queueEvent(ev);
  }

  void press(she::KeyScancode scancode, she::KeyModifiers modifiers, int unicodeChar = 0) {
    key(she::Event::KeyDown, scancode, modifiers, unicodeChar);
    key(she::Event::KeyUp, scancode, modifiers, unicodeChar);
  }

  // A finger can't hover: once it lifts, nothing is under the pointer (no brush preview, no
  // highlighted button left behind). It waits for a pause, so the click before it lands first.
  void leaveSoon() {
    emscripten_async_call([](void* events) {
      if (uintptr_t(events) == pointerEvents && fingers.empty() && !penDown)
        post(she::Event::MouseLeave, 0, 0);
    }, (void*)uintptr_t(pointerEvents), 300);
  }

  void click(double x, double y) {
    post(she::Event::MouseMove, x, y);
    post(she::Event::MouseDown, x, y, she::PointerType::Mouse, 1, she::Event::LeftButton);
    post(she::Event::MouseUp, x, y, she::PointerType::Mouse, 0, she::Event::LeftButton);
    double now = emscripten_get_now();
    if (now - lastTap < kDoubleTapMs && std::hypot(x - lastTapX, y - lastTapY) < 2 * kTapPx) {
      post(she::Event::MouseDoubleClick, x, y, she::PointerType::Mouse, 0, she::Event::LeftButton);
      lastTap = -1e9;
    }
    else {
      lastTap = now;
      lastTapX = x;
      lastTapY = y;
    }
  }

  void startDrawing() {
    g->drawing = g->drew = true;
    post(she::Event::MouseMove, g->x0, g->y0);
    post(she::Event::MouseDown, g->x0, g->y0, she::PointerType::Mouse, 1, she::Event::LeftButton);
    post(she::Event::MouseMove, g->lx, g->ly);
  }

  void stopDrawing() {
    if (!g->drawing)
      return;
    g->drawing = false;
    post(she::Event::MouseUp, g->lx, g->ly, she::PointerType::Mouse, 0, she::Event::LeftButton);
  }

  void endGesture(bool lifted) {
    if (!g)
      return;
    stopDrawing();
    if (lifted && !g->moved && !g->drew) {
      bool quick = emscripten_get_now() - g->t0 < kTapMs;
      // A long press clicks too, until a stylus has been seen: then it's a hand resting.
      if (g->max == 1 && (quick || !stylusSeen))
        click(g->x0, g->y0);
      else if (quick && g->max == 2)
        press(she::kKeyZ, she::kKeyCtrlModifier);
      else if (quick && g->max == 3)
        press(she::kKeyY, she::kKeyCtrlModifier);
    }
    leaveSoon();
    g.reset();
  }

  struct Centre { double x, y, d; };
  Centre centre() {
    auto a = fingers.begin()->second, b = std::next(fingers.begin())->second;
    return {(a.x + b.x) / 2, (a.y + b.y) / 2, std::hypot(a.x - b.x, a.y - b.y)};
  }

  void onPen(int type, double x, double y, float pressure) {
    stylusSeen = true;
    if (type == kPointerDown) {
      endGesture(false);
      fingers.clear();  // whatever was touching (a palm, say) is forgotten until it lifts
      penDown = true;
    }
    if (penDown && type != kPointerUp && type != kPointerCancel) {
      // A stylus that reports no pressure draws at full size.
      penPressure = pressure > 0 ? pressure : 1;
      if (type == kPointerDown) {
        post(she::Event::MouseMove, x, y, she::PointerType::Pen, penPressure);
        post(she::Event::MouseDown, x, y, she::PointerType::Pen, penPressure, she::Event::LeftButton);
      }
      else
        post(she::Event::MouseMove, x, y, she::PointerType::Pen, penPressure);
    }
    else if (penDown) {
      // The stroke ends at the pressure it had (a lifted stylus reports none).
      penDown = false;
      post(she::Event::MouseUp, x, y, she::PointerType::Pen, penPressure, she::Event::LeftButton);
      penPressure = 0;
      leaveSoon();
    }
    else {
      penPressure = 0;
      post(she::Event::MouseMove, x, y, she::PointerType::Pen, 0);  // hovering
    }
  }

  void onFinger(int type, int id, double x, double y) {
    double now = emscripten_get_now();

    if (type == kPointerDown) {
      if (penDown)
        return;  // a palm resting while the stylus draws
      fingers[id] = {x, y};
      if (!g) {
        g.emplace();
        g->t0 = now;
        g->x0 = g->lx = x;
        g->y0 = g->ly = y;
      }
      else if (!g->done) {
        g->max = std::max<int>(g->max, fingers.size());
        stopDrawing();
        if (fingers.size() == 2) {
          Centre c = centre();
          g->cx = c.x;
          g->cy = c.y;
          g->d0 = g->dRef = c.d;
        }
      }
      return;
    }

    auto it = fingers.find(id);
    if (it == fingers.end())
      return;

    if (type == kPointerMove) {
      it->second = {x, y};
      if (!g || g->done)
        return;
      if (fingers.size() == 1 && g->max == 1) {
        g->lx = x;
        g->ly = y;
        if (std::hypot(x - g->x0, y - g->y0) > kTapPx)
          g->moved = true;
        if (g->drawing)
          post(she::Event::MouseMove, x, y, she::PointerType::Mouse, 1);
        else if (!stylusSeen && g->moved && now - g->t0 >= kHoldMs)
          startDrawing();
      }
      else if (fingers.size() >= 2) {
        // The first clear movement decides: fingers spreading or closing zoom, fingers
        // sliding together scroll. Doing both at once makes the canvas jump.
        Centre c = centre();
        if (g->mode == Gesture::Undecided) {
          double slide = std::hypot(c.x - g->cx, c.y - g->cy);
          double spread = std::abs(c.d - g->d0);
          if (slide <= kTapPx && spread <= kTapPx)
            return;
          g->moved = true;
          g->mode = (spread > slide ? Gesture::Zoom : Gesture::Scroll);
          g->px = g->cx;
          g->py = g->cy;
          post(she::Event::MouseMove, c.x, c.y);  // the editor under the fingers takes the gesture
        }
        if (g->mode == Gesture::Scroll) {
          // The canvas follows the fingers.
          g->sx += g->px - c.x;
          g->sy += g->py - c.y;
          g->px = c.x;
          g->py = c.y;
          int scale = displayScale();
          gfx::Point delta(int(g->sx / scale), int(g->sy / scale));
          if (delta.x || delta.y) {
            g->sx -= delta.x * scale;
            g->sy -= delta.y * scale;
            she::Event ev;
            ev.setType(she::Event::MouseWheel);
            ev.setPosition(gfx::Point(int(c.x) / scale, int(c.y) / scale));
            ev.setModifiers(she::kKeyNoneModifier);
            ev.setPointerType(she::PointerType::Multitouch);
            ev.setWheelDelta(delta);
            ev.setPreciseWheel(true);
            she::EventQueue::instance()->queueEvent(ev);
          }
        }
        else if (std::abs(c.d / g->dRef - 1) > 0.02) {
          she::Event ev;
          ev.setType(she::Event::TouchMagnify);
          ev.setPosition(gfx::Point(int(c.x) / displayScale(), int(c.y) / displayScale()));
          ev.setModifiers(she::kKeyNoneModifier);
          ev.setMagnification(c.d / g->dRef - 1);
          she::EventQueue::instance()->queueEvent(ev);
          g->dRef = c.d;
        }
      }
      return;
    }

    // Up or cancel
    fingers.erase(it);
    if (!g)
      return;
    if (type == kPointerCancel)
      g->moved = true;  // the system took the touch (an edge swipe, say)
    if (fingers.empty())
      endGesture(true);
    else if (fingers.size() < 2 && g->max >= 2)
      g->done = true;  // the rest just waits to lift
  }

  she::KeyScancode scancodeFromKeyCode(int keyCode) {
    if (keyCode >= 'A' && keyCode <= 'Z')
      return she::KeyScancode(she::kKeyA + keyCode - 'A');
    if (keyCode >= '0' && keyCode <= '9')
      return she::KeyScancode(she::kKey0 + keyCode - '0');
    switch (keyCode) {
      case 8:  return she::kKeyBackspace;
      case 9:  return she::kKeyTab;
      case 13: return she::kKeyEnter;
      case 27: return she::kKeyEsc;
      case 35: return she::kKeyEnd;
      case 36: return she::kKeyHome;
      case 37: return she::kKeyLeft;
      case 38: return she::kKeyUp;
      case 39: return she::kKeyRight;
      case 40: return she::kKeyDown;
      case 46: return she::kKeyDel;
    }
    return she::kKeyNil;
  }
}

extern "C" {
  void onTouchPointer(int type, int id, int isPen, double x, double y, float pressure) {
    ++pointerEvents;
    if (isPen)
      onPen(type, x, y, pressure);
    else
      onFinger(type, id, x, y);
  }

  // Text from the on-screen keyboard: letters, autocorrect, dictation. It goes straight to the
  // queue, one key at a time and in order, rather than through SDL's text input.
  void onTypedText(const char* text) {
    std::string str = text;
    base::utf8_const_iterator it(str.begin()), end(str.end());
    for (; it != end; ++it)
      press(she::kKeyNil, she::kKeyNoneModifier, *it);
  }

  // Backspace, Enter, the arrows and the like, and shortcuts from a keyboard attached to the
  // tablet while the on-screen keyboard is up.
  void onTypedKey(int down, int keyCode, int ctrl, int shift, int alt) {
    she::KeyScancode scancode = scancodeFromKeyCode(keyCode);
    if (scancode == she::kKeyNil)
      return;
    int modifiers = (ctrl ? she::kKeyCtrlModifier : 0)
                  | (shift ? she::kKeyShiftModifier : 0)
                  | (alt ? she::kKeyAltModifier : 0);
    if (down) {
      key(she::Event::KeyDown, scancode, she::KeyModifiers(modifiers));
      return;
    }
    // A tablet's keyboard sends a key's press and release together, but the interface acts on
    // some releases after the press has moved the focus (Enter moves it to the default button,
    // which clicks on the release). So the release comes a moment later.
    emscripten_async_call([](void* arg) {
      auto packed = uintptr_t(arg);
      key(she::Event::KeyUp, she::KeyScancode(packed & 0xffff), she::KeyModifiers(packed >> 16));
    }, (void*)(uintptr_t(scancode) | uintptr_t(modifiers) << 16), 50);
  }
}

#endif
namespace she {
  void log(const std::string& text) {
#if defined(ANDROID)
    SDL_Log("%s", text.c_str());
#endif
  }

  namespace sdl {
    bool isMaximized;
    bool isMinimized;
    extern std::unordered_map<int, SDL2Display*> windowIdToDisplay;
  }

  class SDL2EventQueue : public EventQueue {
  public:
    PointerType pointerType = PointerType::Mouse;
    std::chrono::steady_clock::time_point lastUpTime = std::chrono::steady_clock::now();

    SDL2EventQueue() {
#if defined(__EMSCRIPTEN__)
        setup_touch_and_typing();
#endif
      if (reverseKeyCodeMapping.empty()) {
        for (auto& entry : keyCodeMapping) {
          reverseKeyCodeMapping[entry.second.sheModifier] = &entry.second;
          entry.second.ascii = entry.first;
        }
      }
    }

    void forceFlip() {
      for (auto& entry : sdl::windowIdToDisplay) {
        entry.second->flip({
            0,
            0,
            entry.second->width(),
            entry.second->height()
          });
        entry.second->present();
      }
    }

    void refresh() {
      if (!m_events.empty())
	return;
      Event event;
      while (true) {
	event.setType(Event::None);
	getEventInternal(event, false);
	if (event.type() == Event::None) {
	  return;
	}
	m_events.push(event);
      }
    }

    void getEvent(Event& event, bool) override {
      event.setType(Event::None);
      if (m_events.try_pop(event))
        return;
      if (she::instance()->isGfxThread())
	getEventInternal(event, false);
    }

    void getEventInternal(Event& event, bool) {
      SDL_Event sdlEvent;
      while (SDL_PollEvent(&sdlEvent)) {
        switch (sdlEvent.type) {
        case SDL_APP_DIDENTERFOREGROUND:
          SDL2Surface::textureGen++;
          forceFlip();
          continue;

	case SDL_SYSWMEVENT:
#if defined(EASYTAB_H)
#if defined(_WIN32)
	  {
	    auto& win = sdlEvent.syswm.msg->msg.win;
	    if (EasyTab_HandleEvent(win.hwnd, win.msg, win.lParam, win.wParam) == EASYTAB_OK) {
		penPressure = std::max<>(EasyTab->Pressure, 0.0001f);
	    }
	  }
#endif
#if defined(__linux__)
	    if (EasyTab_HandleEvent(&sdlEvent.syswm.msg->msg.x11.event) == EASYTAB_OK) {
		penPressure = std::max(EasyTab->Pressure, 0.0001f);
	    }
#endif
#endif
	    continue;

        case SDL_WINDOWEVENT:
          switch (sdlEvent.window.event) {
          case SDL_WINDOWEVENT_EXPOSED:
            forceFlip();
            continue;
          case SDL_WINDOWEVENT_SIZE_CHANGED:
            continue;

          case SDL_WINDOWEVENT_MAXIMIZED:
            sdl::isMaximized = true;
            sdl::isMinimized = false;
            std::cout << "Maximized" << std::endl;
            continue;

          case SDL_WINDOWEVENT_MINIMIZED:
            sdl::isMaximized = false;
            sdl::isMinimized = true;
            std::cout << "Minimized" << std::endl;
            continue;

          case SDL_WINDOWEVENT_RESTORED:
            sdl::isMaximized = false;
            sdl::isMinimized = false;
            std::cout << "Restored" << std::endl;
            continue;

          case SDL_WINDOWEVENT_RESIZED: {
	    #ifdef __EMSCRIPTEN__
	    continue;
	    #else
            auto display = sdl::windowIdToDisplay[sdlEvent.window.windowID];
            display->setWidth(sdlEvent.window.data1);
            display->setHeight(sdlEvent.window.data2);
            display->recreateSurface();
            event.setType(Event::ResizeDisplay);
            event.setDisplay(display);
            return;
	    #endif
          }

          case SDL_WINDOWEVENT_LEAVE: {
            if (display_has_mouse) {
              display_has_mouse = false;

              Event ev;
              ev.setType(Event::MouseLeave);
              m_events.push(ev);
              break;
            }
          }
          //silence 'Unknown windowevent' console spam for common SDL window events
          case SDL_WINDOWEVENT_SHOWN:
          case SDL_WINDOWEVENT_HIDDEN:
          case SDL_WINDOWEVENT_MOVED:
          case SDL_WINDOWEVENT_ENTER:
          case SDL_WINDOWEVENT_FOCUS_GAINED:
          case SDL_WINDOWEVENT_FOCUS_LOST:
          case SDL_WINDOWEVENT_CLOSE: 
          //closing the app is handled elsewhere so we can ignore it here
            continue;

          default:
            std::cout << "Unknown windowevent: " << (int) sdlEvent.window.event << std::endl;
            continue;
          }
          continue;

        case SDL_MOUSEMOTION:
          if (!display_has_mouse) {
            display_has_mouse = true;
            Event ev;
            ev.setType(Event::MouseEnter);
            m_events.push(ev);
          }

          event.setType(Event::MouseMove);
          event.setModifiers(getSheModifiers());
          event.setPosition({
              sdlEvent.motion.x / unique_display->scale(),
              sdlEvent.motion.y / unique_display->scale()
            });

	  {
	      int hasFingerEvent = SDL_PeepEvents(&sdlEvent, 1, SDL_PEEKEVENT, SDL_FINGERMOTION, SDL_FINGERMOTION);
	      if (hasFingerEvent) {
		  penPressure = std::max<>(sdlEvent.tfinger.pressure, 0.0001f);
	      }
	  }


	  event.setPressure(penPressure);
	  event.setPointerType(pointerType);
          return;

        case SDL_FINGERMOTION:
#ifndef __EMSCRIPTEN__
          penPressure = std::max<>(sdlEvent.tfinger.pressure, 0.0001f);
#endif
          continue;

#ifdef __EMSCRIPTEN__
        case SDL_FINGERDOWN:
        case SDL_FINGERUP:
          continue;  // fingers come as pointer events instead (onTouchPointer)
#endif

        case SDL_MOUSEWHEEL:
          event.setType(Event::MouseWheel);
          event.setModifiers(getSheModifiers());
          event.setWheelDelta({-sdlEvent.wheel.x, -sdlEvent.wheel.y});
          int x, y;
          SDL_GetMouseState(&x, &y);
          event.setPosition({
              x / unique_display->scale(),
              y / unique_display->scale()
            });
          return;

        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN: {
          auto type = sdlEvent.type == SDL_MOUSEBUTTONDOWN ? Event::MouseDown : Event::MouseUp;
          event.setType(type);
          event.setPosition({
              sdlEvent.button.x / unique_display->scale(),
              sdlEvent.button.y / unique_display->scale()
            });
          event.setButton(mouseButtonMapping[sdlEvent.button.button]);
          event.setModifiers(getSheModifiers());

	  if (penPressure > 0.0f) {
	    pointerType = PointerType::Pen;
	    event.setPressure(penPressure);
	    event.setPointerType(pointerType);
	  } else {
	    event.setPressure(sdlEvent.type == SDL_MOUSEBUTTONDOWN ? 1.0f : 0.0f);
	    event.setPointerType(pointerType);
	    pointerType = PointerType::Mouse;
	  }

	  auto now = std::chrono::steady_clock::now();
	  auto delta = now - lastUpTime;
          if (sdlEvent.type == SDL_MOUSEBUTTONUP) {
	    using namespace std::chrono_literals;
	    if (delta < 200ms) {
	      m_events.push(event);
	      event.setType(Event::MouseDoubleClick);
	      event.setPosition(event.position());
	      event.setButton(event.button());
	    }
	    lastUpTime = now;
          }

          return;
        }

        case SDL_KEYDOWN:
        case SDL_KEYUP: {
          Event event;
          bool isPressed = sdlEvent.type == SDL_KEYDOWN;
          auto modifierIt = modifiers.find((SDL_Keycode) sdlEvent.key.keysym.sym);
          if (modifierIt != modifiers.end()) {
            modifierIt->second.isPressed = sdlEvent.type == SDL_KEYDOWN;
          }

          auto it = keyCodeMapping.find((SDL_Keycode) sdlEvent.key.keysym.sym);

          if (it == keyCodeMapping.end()) {
            std::cout << "Unknown scancode: " << sdlEvent.key.keysym.sym << std::endl;
            continue;
          }

          event.setType(isPressed ? Event::KeyDown : Event::KeyUp);
          auto modifiers = getSheModifiers();
          event.setModifiers(modifiers);
          it->second.isPressed = isPressed;
          auto scancode = static_cast<she::KeyScancode>(it->second.sheModifier);
          event.setScancode(scancode);
          if (isPressed) {
            lastScancode = scancode;
            lastScancodeSDL = sdlEvent.key.keysym.scancode;
          }
          if (sdlEvent.key.repeat) {
            event.setRepeat(sdlEvent.key.repeat);
          }
          keybuffer.push_back(event);
          if (modifiers & (she::kKeyCtrlModifier | she::kKeyCmdModifier)) {
            SDL_StopTextInput();
            break;
          } else if (!SDL_IsTextInputActive()) {
            SDL_StartTextInput();
          }
          continue;
        }

        case SDL_DROPFILE: {
          std::string file(sdlEvent.drop.file);
          event.setType(Event::DropFiles);
          event.setFiles({file});
          SDL_free(sdlEvent.drop.file);
          return;
        }

          // CloseDisplay,
          // ResizeDisplay,
          // MouseEnter,
          // MouseLeave,
          // TouchMagnify,
        case SDL_QUIT:
          event.setType(Event::CloseDisplay);
          return;

        case SDL_TEXTEDITING:
          continue;

        case SDL_TEXTINPUT: {
          keybuffer.clear();
          std::string textString = sdlEvent.text.text;
          base::utf8_const_iterator begin{textString.begin()};
          base::utf8_const_iterator end{textString.end()};
          Event event;
          event.setModifiers(getSheModifiers());
          for (auto it = begin; it != end; ++it) {
            event.setType(Event::KeyDown);
            event.setUnicodeChar(*it);
            if (lastScancodeSDL > SDL_SCANCODE_UNKNOWN && lastScancodeSDL < SDL_SCANCODE_RETURN) {
              event.setScancode(lastScancode);
              lastScancodeSDL = SDL_SCANCODE_UNKNOWN;
            }
            keybuffer.push_back(event);
            event.setType(Event::KeyUp);
            keybuffer.push_back(event);
          }

          break;
        }

        case SDL_KEYMAPCHANGED:
          continue;

        default:
          std::cout << "Unknown event: " << sdlEvent.type << std::endl;
          continue;
        }
      }

      if (!keybuffer.empty()) {
        event = keybuffer.front();
        keybuffer.pop_front();
        return;
      }
    }

    void queueEvent(const Event& event) override {
      m_events.push(event);
    }

  private:
    base::concurrent_queue<Event> m_events;
  };

  EventQueue* EventQueue::instance() {
    static SDL2EventQueue g_queue;
    return &g_queue;
  }

  class SDL2System : public CommonSystem {
  public:
    SDL2System() {
      g_instance = this;
    }

    ~SDL2System() {
      shutdown = true;
      sleeping = false;
      if (mainThread.joinable())
	mainThread.join();
      IMG_Quit();
      SDL_Quit();
      g_instance = nullptr;
    }

    bool shutdown{false};
    std::thread mainThread;
    std::thread::id mainThreadId;
    std::thread::id gfxThreadId;
    std::vector<std::function<void()>> gfxQueue;
    std::atomic<bool> sleeping{false};

    bool isGfxThread() override {
      return std::this_thread::get_id() == gfxThreadId;
    }

    bool isMainThread() override {
      return std::this_thread::get_id() == mainThreadId;
    }

    void gfx(std::function<void()>&& func, bool sleep) override {
      if (isGfxThread()) {
	func();
	return;
      }
      gfxQueue.emplace_back(std::move(func));
      if (sleep)
	this->sleep();
    }

    using Timestamp = std::chrono::high_resolution_clock::time_point;
    Timestamp start = std::chrono::high_resolution_clock::now();

    void sleep() override {
      using namespace std::chrono_literals;
      if (shutdown)
	return;

      if (mainThreadId == gfxThreadId) {
	refresh();
	auto now = std::chrono::high_resolution_clock::now();

	// If the dispatching of messages was faster than 10 milliseconds,
	// it means that the process is not using a lot of CPU, so we can
	// wait the difference to cover those 10 milliseconds
	// sleeping. With this code we can avoid 100% CPU usage.
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
	start = now;

	if (elapsed < 15ms)
	  std::this_thread::sleep_for(15ms - elapsed);
      } else if (isMainThread()) {
	sleeping = true;
	while (sleeping) {
	  using namespace std::chrono_literals;
	  std::this_thread::sleep_for(10ms);
	}
      }
    }

    std::function<int()> m_func;
    int run(std::function<int()>&& func) override {
      gfxThreadId = std::this_thread::get_id();
      #ifndef EMSCRIPTEN
      mainThreadId = gfxThreadId;
      return func();
      #elif defined(EMSCRIPTEN) && !defined(__EMSCRIPTEN__)
      m_func = std::move(func);
      // like emscripten, but not really
      mainThread = std::thread{[this]{
        mainThreadId = std::this_thread::get_id();
	m_func();
      }};
      while (!shutdown)(
	refresh();
      }
      #else
      m_func = std::move(func);

      addEventListener("dragenter", cancelEvent);
      addEventListener("dragover", cancelEvent);
      addEventListener("drop", [](void*){
	EM_ASM({
	  event.stopPropagation();
	  event.preventDefault();
	  let files = event.dataTransfer?.files;
	  if (files?.length) {
	    for (let i = 0; i < files.length; ++i) {
	      let fr = new FileReader();
	      fr.onload = ((fr, name)=>{
		  try{ FS.mkdir('/tmp'); }catch(ex){}
		  FS.writeFile('/tmp/' + name, new Uint8Array(fr.result));
		  canvas.dispatchEvent(new CustomEvent("readFile", {detail:{path:'/tmp/' + name}}));
		}).bind(null, fr, files[i].name);
	      fr.readAsArrayBuffer(files[i]);
	    }
	  }
        });
      });

      addEventListener("readFile", [](void*){
	auto str = (char*) EM_ASM_PTR({return stringToNewUTF8(event.detail.path)});
	std::string path = str;
	free(str);
	Event event;
	event.setType(Event::DropFiles);
	event.setFiles({path});
	static_cast<SDL2EventQueue*>(EventQueue::instance())->queueEvent(event);
      });

      emscripten_set_main_loop([]{
	  if (!cfginit())
	      return;
	  auto sys = static_cast<SDL2System*>(g_instance);
	  sys->mainThread = std::thread{[]{
	      auto sys = static_cast<SDL2System*>(g_instance);
	      sys->mainThreadId = std::this_thread::get_id();
	      sys->m_func();
	  }};
	  emscripten_cancel_main_loop();
	  emscripten_set_main_loop([]{
	      patchEventListeners();
	      static_cast<SDL2System*>(g_instance)->refresh();
	  }, 0, true);
      }, 0, true);
      #endif
      return 0;
    }

    void refresh() {
      if (!sleeping) {
	static_cast<SDL2EventQueue*>(EventQueue::instance())->refresh();
	return;
      }
      #ifdef __EMSCRIPTEN__
      auto width = get_canvas_width();
      auto height = get_canvas_height();
      if (width && height && (oldWidth != width || oldHeight != height) && !sdl::windowIdToDisplay.empty()) {
	oldWidth = width;
	oldHeight = height;
	for (auto& entry : sdl::windowIdToDisplay) {
	  auto display = entry.second;
	  SDL_SetWindowSize(display->m_window, width, height);
	  display->setWidth(width);
	  display->setHeight(height);
	  display->recreateSurface();
	  Event event;
	  event.setType(Event::ResizeDisplay);
	  event.setDisplay(display);
	  static_cast<SDL2EventQueue*>(EventQueue::instance())->queueEvent(event);
	}
      }
      #endif
      int frames = 5;
      do {
	for (auto it = gfxQueue.begin(); it != gfxQueue.end(); ++it) {
	  (*it)();
	}
	gfxQueue.clear();
	sleeping = false;
	for (auto& entry : sdl::windowIdToDisplay)
	  entry.second->present();
	static_cast<SDL2EventQueue*>(EventQueue::instance())->refresh();
      } while (sleeping && --frames);
    }

    void activateApp() override {
      // Do nothing
    }

    void finishLaunching() override {
      // Do nothing
    }

    Capabilities capabilities() const override {
      return (Capabilities)(int(Capabilities::CanResizeDisplay) | int(Capabilities::GpuAccelerationSwitch));
    }

    EventQueue* eventQueue() override { // TODO remove this function
      return EventQueue::instance();
    }

    bool gpuAcceleration() const override {
      return SDL2Display::gpu;
    }

    void setGpuAcceleration(bool state) override {
      if (!unique_display)
        SDL2Display::gpu = state;
    }

    gfx::Size defaultNewDisplaySize() override {
      return gfx::Size(0, 0);
    }

    gfx::Size desktopSize() override {
      // Reports the primary display's resolution in pixels (or in points on
      // platforms where the window is not created high-DPI aware, e.g. macOS
      // without SDL_WINDOW_ALLOW_HIGHDPI). Returns (0, 0) when SDL can't
      // determine it or the video subsystem isn't up yet.
      if (SDL_WasInit(SDL_INIT_VIDEO) == 0)
        return gfx::Size(0, 0);
      SDL_DisplayMode mode;
      if (SDL_GetDesktopDisplayMode(0, &mode) != 0)
        return gfx::Size(0, 0);
      return gfx::Size(mode.w, mode.h);
    }

    Display* defaultDisplay() override {
      return unique_display;
    }

    Display* createDisplay(int width, int height, int scale) override {
      //LOG("Creating display %dx%d (scale = %d)\n", width, height, scale);
      return new SDL2Display(width, height, scale);
    }

    Surface* createSurface(int width, int height) override {
      return new SDL2Surface(width, height, SDL2Surface::DeleteAndDestroy);
    }

    Surface* createRgbaSurface(int width, int height) override {
      return new SDL2Surface(width, height, 32, SDL2Surface::DeleteAndDestroy);
    }

    std::vector<uint8_t> encodeSurfaceAsPNG(Surface* s) override {
      auto surface = static_cast<SDL2Surface*>(s);
      std::vector<uint8_t> data;
      data.resize(surface->width() * surface->height() * 4 + 1024);
      std::shared_ptr<SDL_RWops> rops{
        SDL_RWFromMem(data.data(), data.size()),
        [](auto *rops){ rops->close(rops); }
      };
      if (IMG_SavePNG_RW(static_cast<SDL_Surface*>(surface->nativeHandle()), rops.get(), 0) != 0)
        return {};
      data.resize(SDL_RWtell(rops.get()));
      return data;
    }

    Surface* loadSurface(const char* filename) override {
      SDL_Surface* bmp = IMG_Load(filename);
      if (!bmp)
	throw std::runtime_error(std::string{"Error loading image "} + filename);
      return new SDL2Surface(bmp, SDL2Surface::DeleteAndDestroy);
    }

    Surface* loadRgbaSurface(const char* filename) override {
      SDL_Surface* bmp = IMG_Load(filename);
      if (!bmp)
	throw std::runtime_error(std::string{"Error loading image "} + filename);
      if (bmp->format->BitsPerPixel < 32) {
        auto copy = SDL_ConvertSurfaceFormat(bmp, SDL_PIXELFORMAT_RGBA8888, 0);
        SDL_FreeSurface(bmp);
        bmp = copy;
      }
      return new SDL2Surface(bmp, SDL2Surface::DeleteAndDestroy);
    }

  };

  System* create_system() {
    return new SDL2System();
  }

  System* instance()
  {
    return g_instance;
  }

  void error_message(const char* msg)
  {
    if (g_instance && g_instance->logger())
      g_instance->logger()->logError(msg);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, PACKAGE, msg, nullptr);
  }

  int scancode_to_ascii(KeyScancode scancode) {
    auto it = reverseKeyCodeMapping.find(scancode);
    if (it == reverseKeyCodeMapping.end())
      return 0;
    return it->second->ascii;
  }

  bool is_key_pressed(KeyScancode scancode) {
    auto it = reverseKeyCodeMapping.find(scancode);
    if (it != reverseKeyCodeMapping.end()) {
      return it->second->isPressed;
    }
    return false;
  }

  void set_input_rect(const gfx::Rect& rect) {
#ifdef __EMSCRIPTEN__
    // Where the focused text box is, so a tap in it brings up a tablet's keyboard.
    MAIN_THREAD_ASYNC_EM_ASM({ if (Module.setTextRect) Module.setTextRect($0, $1, $2, $3); },
                             rect.x, rect.y, rect.w, rect.h);
#endif
    if (rect.isEmpty()) {
      SDL_StopTextInput();
      return;
    }
    SDL_Rect sdlRect{
      rect.x,
      rect.y,
      rect.w,
      rect.h
    };
    SDL_SetTextInputRect(&sdlRect);
    SDL_StartTextInput();
  }

  void clear_keyboard_buffer() {
    keybuffer.clear();
  }

} // namespace she

// It must be defined by the user program code.
extern int app_main(int argc, char* argv[]);

int main(const int argc, char* argv[]) {
  #ifdef SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR
  SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
  #endif
  SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");

  // If the requested DPI awareness is not available on the currently running OS,
  // SDL will try to request the best available match.
  // https://wiki.libsdl.org/SDL2/SDL_HINT_WINDOWS_DPI_AWARENESS
  SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");

#ifdef __EMSCRIPTEN__
  // Fingers become gestures (onTouchPointer), not a mouse.
  SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
#endif

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    std::cerr << "Critical: Could not initialize SDL2. Aborting." << std::endl;
    return -1;
  }
  if (!IMG_Init(-1)) {
    std::cerr << "Critical: Could not initialize SDL2_image (" << IMG_GetError() << "). Aborting." << std::endl;
    return -2;
  }
  SDL_EventState(SDL_FINGERMOTION, SDL_ENABLE);
  return app_main(argc, argv);
}
