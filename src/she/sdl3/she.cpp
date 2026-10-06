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
#include "she/sdl3/sdl3_display.h"
#include "she/sdl3/sdl3_surface.h"
#include "she/common/system.h"
#include "she/logger.h"

#undef HAVE_STDINT_H
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <list>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

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
  {SDLK_A, she::kKeyA},
  {SDLK_B, she::kKeyB},
  {SDLK_C, she::kKeyC},
  {SDLK_D, she::kKeyD},
  {SDLK_E, she::kKeyE},
  {SDLK_F, she::kKeyF},
  {SDLK_G, she::kKeyG},
  {SDLK_H, she::kKeyH},
  {SDLK_I, she::kKeyI},
  {SDLK_J, she::kKeyJ},
  {SDLK_K, she::kKeyK},
  {SDLK_L, she::kKeyL},
  {SDLK_M, she::kKeyM},
  {SDLK_N, she::kKeyN},
  {SDLK_O, she::kKeyO},
  {SDLK_P, she::kKeyP},
  {SDLK_Q, she::kKeyQ},
  {SDLK_R, she::kKeyR},
  {SDLK_S, she::kKeyS},
  {SDLK_T, she::kKeyT},
  {SDLK_U, she::kKeyU},
  {SDLK_V, she::kKeyV},
  {SDLK_W, she::kKeyW},
  {SDLK_X, she::kKeyX},
  {SDLK_Y, she::kKeyY},
  {SDLK_Z, she::kKeyZ},
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
  {SDLK_APOSTROPHE, she::kKeyTilde},
  {SDLK_MINUS, she::kKeyMinus},
  {SDLK_EQUALS, she::kKeyEquals},
  {SDLK_BACKSPACE, she::kKeyBackspace},
  {SDLK_TAB, she::kKeyTab},
  {SDLK_LEFTBRACKET, she::kKeyOpenbrace},
  {SDLK_RIGHTBRACKET, she::kKeyClosebrace},
  {SDLK_KP_ENTER, she::kKeyEnter},
  {SDLK_COLON, she::kKeyColon},
  {SDLK_APOSTROPHE, she::kKeyQuote},
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
  {SDLK_GRAVE, she::kKeyBackquote},
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
namespace she {
  void log(const std::string& text) {
#if defined(ANDROID)
    SDL_Log("%s", text.c_str());
#endif
  }

  namespace sdl {
    bool isMaximized;
    bool isMinimized;
    extern std::unordered_map<int, SDL3Display*> windowIdToDisplay;
  }

  class SDL3EventQueue : public EventQueue {
  public:
    PointerType pointerType = PointerType::Mouse;
    std::chrono::steady_clock::time_point lastUpTime = std::chrono::steady_clock::now();

    SDL3EventQueue() {
#if defined(__EMSCRIPTEN__)
        EM_ASM(
            const onPointerEvent = Module.cwrap("onPointerEvent", "", ["number", "number"]);
	    const listener = event => {
		onPointerEvent((event.pointerType == "pen")|0, event.pressure);
	    };
            Module.canvas.addEventListener("pointerdown", listener);
	    Module.canvas.addEventListener("pointermove", listener);
	    Module.canvas.addEventListener("pointerup", listener);
            );
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
        case SDL_EVENT_DID_ENTER_FOREGROUND:
          SDL3Surface::textureGen++;
          forceFlip();
          continue;
        case SDL_EVENT_WINDOW_EXPOSED:
          forceFlip();
          continue;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
          continue;

        case SDL_EVENT_WINDOW_MAXIMIZED:
          sdl::isMaximized = true;
          sdl::isMinimized = false;
          std::cout << "Maximized" << std::endl;
          continue;

        case SDL_EVENT_WINDOW_MINIMIZED:
          sdl::isMaximized = false;
          sdl::isMinimized = true;
          std::cout << "Minimized" << std::endl;
          continue;

        case SDL_EVENT_WINDOW_RESTORED:
          sdl::isMaximized = false;
          sdl::isMinimized = false;
          std::cout << "Restored" << std::endl;
          continue;

        case SDL_EVENT_WINDOW_RESIZED: {
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
        case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
          if (display_has_mouse) {
            display_has_mouse = false;
            Event ev;
            ev.setType(Event::MouseLeave);
            m_events.push(ev);
            break;
          }
        }
        //silence 'Unknown windowevent' console spam for common SDL window events
        case SDL_EVENT_WINDOW_SHOWN:
        case SDL_EVENT_WINDOW_HIDDEN:
        case SDL_EVENT_WINDOW_MOVED:
        case SDL_EVENT_WINDOW_MOUSE_ENTER:
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        //closing the app is handled elsewhere so we can ignore it here
          continue;

        case SDL_EVENT_MOUSE_MOTION:
          if (!display_has_mouse) {
            display_has_mouse = true;
            Event ev;
            ev.setType(Event::MouseEnter);
            m_events.push(ev);
          }

          event.setType(Event::MouseMove);
          event.setModifiers(getSheModifiers());
          event.setPosition(gfx::Point(
            sdlEvent.motion.x / unique_display->scale(),
            sdlEvent.motion.y / unique_display->scale()
          ));
	        {
	          int hasFingerEvent = SDL_PeepEvents(&sdlEvent, 1, SDL_PEEKEVENT, SDL_EVENT_FINGER_MOTION, SDL_EVENT_FINGER_MOTION);
	          if (hasFingerEvent) {
		          penPressure = std::max<>(sdlEvent.tfinger.pressure, 0.0001f);
	          }
	        }


	  if (sdlEvent.motion.which == SDL_PEN_MOUSEID) {
	    pointerType = PointerType::Pen;
	    if (penPressure == 0.0f)
	      penPressure = 0.0001f;
	  }

	  event.setPressure(penPressure);
	  event.setPointerType(pointerType);
          return;

        case SDL_EVENT_FINGER_MOTION:
          penPressure = std::max<>(sdlEvent.tfinger.pressure, 0.0001f);
          continue;

        case SDL_EVENT_PEN_AXIS:
          if (sdlEvent.paxis.axis == SDL_PEN_AXIS_PRESSURE)
            penPressure = std::max(sdlEvent.paxis.value, 0.0001f);
          continue;

        case SDL_EVENT_PEN_PROXIMITY_OUT:
          // Stops a later plain mouse click from being misreported as pen input with stale pressure once a pen has touched the tablet.
          penPressure = 0.0f;
          pointerType = PointerType::Mouse;
          continue;

        // Position/clicks for these already arrive as ordinary SDL_EVENT_MOUSE_* events
        // (SDL_HINT_PEN_MOUSE_EVENTS defaults to enabled), so only the pressure is tracked here.
        case SDL_EVENT_PEN_PROXIMITY_IN:
        case SDL_EVENT_PEN_MOTION:
          // A hovering pen sends no pressure axis events, so mark it as present here;
          // get_pen_pressure() != 0 is what hides the brush preview (and flags pen input) while hovering.
          if (penPressure == 0.0f)
            penPressure = 0.0001f;
          continue;

        case SDL_EVENT_PEN_DOWN:
        case SDL_EVENT_PEN_UP:
        case SDL_EVENT_PEN_BUTTON_DOWN:
        case SDL_EVENT_PEN_BUTTON_UP:
          continue;

        case SDL_EVENT_MOUSE_WHEEL:
          event.setType(Event::MouseWheel);
          event.setModifiers(getSheModifiers());
          event.setWheelDelta(gfx::Point(-sdlEvent.wheel.x, -sdlEvent.wheel.y));
          float x, y;
          SDL_GetMouseState(&x, &y);
          event.setPosition(gfx::Point(
            x / unique_display->scale(),
            y / unique_display->scale()
          ));
          return;

        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
          auto type = sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? Event::MouseDown : Event::MouseUp;
          event.setType(type);
          event.setPosition(gfx::Point(
              sdlEvent.button.x / unique_display->scale(),
              sdlEvent.button.y / unique_display->scale()
            ));
          event.setButton(mouseButtonMapping[sdlEvent.button.button]);
          event.setModifiers(getSheModifiers());

          // The synthesized mouse event tells us it came from a pen even if no pressure axis event has arrived yet
          // (e.g. first touch after the pen re-entered proximity, which reset penPressure to 0).
          if (sdlEvent.button.which == SDL_PEN_MOUSEID || penPressure > 0.0f) {
            pointerType = PointerType::Pen;
            event.setPressure(std::max(penPressure, 0.0001f));
          } else {
            pointerType = PointerType::Mouse;
            event.setPressure(sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? 1.0f : 0.0f);
          }
          event.setPointerType(pointerType);

          auto now = std::chrono::steady_clock::now();
          auto delta = now - lastUpTime;

          // A double click replaces the second press (the matching release still follows).
          // ui::Widget turns it back into a mouse down, so emitting it after the release caused a press left with no release,
          // which kept a freehand stroke running even with the pen lifted after a quick double tap.
          if (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            using namespace std::chrono_literals;
            if (delta < 200ms) { event.setType(Event::MouseDoubleClick); }
          else {
            lastUpTime = now;
          }

          return;
        }

        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
          Event event;
          bool isPressed = sdlEvent.type == SDL_EVENT_KEY_DOWN;
          auto modifierIt = modifiers.find((SDL_Keycode) sdlEvent.key.key);
          if (modifierIt != modifiers.end()) {
            modifierIt->second.isPressed = sdlEvent.type == SDL_EVENT_KEY_DOWN;
          }

          auto it = keyCodeMapping.find((SDL_Keycode) sdlEvent.key.key);

          if (it == keyCodeMapping.end()) {
            std::cout << "Unknown scancode: " << sdlEvent.key.key << std::endl;
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
            lastScancodeSDL = sdlEvent.key.scancode;
          }
          if (sdlEvent.key.repeat) {
            event.setRepeat(sdlEvent.key.repeat);
          }
          keybuffer.push_back(event);
          SDL_Window* w = SDL_GetWindowFromID(sdlEvent.key.windowID);
          if (modifiers & (she::kKeyCtrlModifier | she::kKeyCmdModifier)) {
            SDL_StopTextInput(w);
            break;
          } else if (!SDL_TextInputActive(w)) {
            SDL_StartTextInput(w);
          }
          continue;
        }

        case SDL_EVENT_DROP_FILE: {
          std::string file(sdlEvent.drop.data);
          event.setType(Event::DropFiles);
          event.setFiles({file});
          // SDL_free(sdlEvent.drop.file); //doesn't sdl own this data?
          return;
        }

          // CloseDisplay,
          // ResizeDisplay,
          // MouseEnter,
          // MouseLeave,
          // TouchMagnify,
        case SDL_EVENT_QUIT:
          event.setType(Event::CloseDisplay);
          return;

        case SDL_EVENT_TEXT_EDITING:
          continue;

        case SDL_EVENT_TEXT_INPUT: {
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

        case SDL_EVENT_KEYMAP_CHANGED:
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
    static SDL3EventQueue g_queue;
    return &g_queue;
  }

  class SDL3System : public CommonSystem {
  public:
    SDL3System() {
      g_instance = this;
    }

    ~SDL3System() {
      shutdown = true;
      sleeping = false;
      if (mainThread.joinable())
	mainThread.join();
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
	static_cast<SDL3EventQueue*>(EventQueue::instance())->queueEvent(event);
      });

      emscripten_set_main_loop([]{
	  if (!cfginit())
	      return;
	  auto sys = static_cast<SDL3System*>(g_instance);
	  sys->mainThread = std::thread{[]{
	      auto sys = static_cast<SDL3System*>(g_instance);
	      sys->mainThreadId = std::this_thread::get_id();
	      sys->m_func();
	  }};
	  emscripten_cancel_main_loop();
	  emscripten_set_main_loop([]{
	      patchEventListeners();
	      static_cast<SDL3System*>(g_instance)->refresh();
	  }, 0, true);
      }, 0, true);
      #endif
      return 0;
    }

    void refresh() {
      if (!sleeping) {
	static_cast<SDL3EventQueue*>(EventQueue::instance())->refresh();
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
	  static_cast<SDL3EventQueue*>(EventQueue::instance())->queueEvent(event);
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
	static_cast<SDL3EventQueue*>(EventQueue::instance())->refresh();
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
      return SDL3Display::gpu;
    }

    void setGpuAcceleration(bool state) override {
      if (!unique_display)
        SDL3Display::gpu = state;
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
      SDL_Window* w = SDL_GetWindows(nullptr)[0];
      SDL_DisplayID id = SDL_GetDisplayForWindow(w);
      const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(id);
      if (mode == nullptr)
        return gfx::Size(0, 0);
      return gfx::Size(mode->w, mode->h);
    }

    Display* defaultDisplay() override {
      return unique_display;
    }

    Display* createDisplay(int width, int height, int scale) override {
      //LOG("Creating display %dx%d (scale = %d)\n", width, height, scale);
      return new SDL3Display(width, height, scale);
    }

    Surface* createSurface(int width, int height) override {
      return new SDL3Surface(width, height, SDL3Surface::DeleteAndDestroy);
    }

    Surface* createRgbaSurface(int width, int height) override {
      return new SDL3Surface(width, height, 32, SDL3Surface::DeleteAndDestroy);
    }

    std::vector<uint8_t> encodeSurfaceAsPNG(Surface* s) override {
      auto surface = static_cast<SDL3Surface*>(s);
      std::vector<uint8_t> data;
      data.resize(surface->width() * surface->height() * 4 + 1024);
      std::shared_ptr<SDL_IOStream> rops{
        SDL_IOFromMem(data.data(), data.size()),
        [](auto *rops){ SDL_CloseIO(rops); }
      };
      if (IMG_SavePNG_IO(static_cast<SDL_Surface*>(surface->nativeHandle()), rops.get(), false) != 0)
        return {};
      data.resize(SDL_TellIO(rops.get()));
      return data;
    }

    Surface* loadSurface(const char* filename) override {
      SDL_Surface* bmp = IMG_Load(filename);
      if (!bmp)
	throw std::runtime_error(std::string{"Error loading image "} + filename);
      return new SDL3Surface(bmp, SDL3Surface::DeleteAndDestroy);
    }

    Surface* loadRgbaSurface(const char* filename) override {
      SDL_Surface* bmp = IMG_Load(filename);
      if (!bmp)
	throw std::runtime_error(std::string{"Error loading image "} + filename);
      auto format = SDL_GetPixelFormatDetails(bmp->format);
      if (format->bits_per_pixel < 32) {
        auto copy = SDL_ConvertSurface(bmp, SDL_PIXELFORMAT_RGBA8888);
        SDL_DestroySurface(bmp);
        bmp = copy;
      }
      return new SDL3Surface(bmp, SDL3Surface::DeleteAndDestroy);
    }

  };

  System* create_system() {
    return new SDL3System();
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
    SDL_Window *window = SDL_GetWindows(nullptr)[0];
    if (rect.isEmpty()) {
      SDL_StopTextInput(window);
      return;
    }
    SDL_Rect sdlRect{
      rect.x,
      rect.y,
      rect.w,
      rect.h
    };
    SDL_SetTextInputArea(window, &sdlRect, 0);
    SDL_StartTextInput(window);
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

  // Pen input arrives as SDL_EVENT_PEN_* events (pressure) plus synthesized mouse events (position/clicks).
  // Also synthesizing touch events for the same pen would fight the finger-based pressure tracking with a second, less precise value.
  SDL_SetHint(SDL_HINT_PEN_TOUCH_EVENTS, "0");

  //To do: consider hinting `wayland,x11` as default SDL Video Driver on Wayland once clippy is replaced with SDL3 clipboard

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == false) {
    std::cerr << "Critical: Could not initialize SDL3. Aborting." << std::endl;
    return -1;
  }
  SDL_SetEventEnabled(SDL_EVENT_FINGER_MOTION, true);


  return app_main(argc, argv);
}
