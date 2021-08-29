// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/she.h"

#include "base/concurrent_queue.h"
#include "base/exception.h"
#include "base/string.h"
#include "base/unique_ptr.h"
#include "she/sdl2/sdl2_display.h"
#include "she/sdl2/sdl2_surface.h"
#include "she/common/system.h"
#include "she/logger.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <cassert>
#include <list>
#include <vector>
#include <unordered_map>

#include "she/sdl2/display_events.h"
#ifdef USE_KEY_POLLER
#include "she/sdl2/key_poller.h"
#endif
#ifdef USE_MOUSE_POLLER
#include "she/sdl2/mouse_poller.h"
#endif

static she::System* g_instance = nullptr;
static std::unordered_map<int, she::Event::MouseButton> mouseButtonMapping = {
    {SDL_BUTTON_LEFT, she::Event::LeftButton},
    {SDL_BUTTON_MIDDLE, she::Event::MiddleButton},
    {SDL_BUTTON_RIGHT, she::Event::RightButton}
};

static std::unordered_map<SDL_KeyCode, she::KeyScancode> keyCodeMapping = {
    {SDLK_UNKNOWN, she::kKeyNil},
    {SDL_KeyCode(13), she::kKeyEnter},
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
    // {SDLK_0PAD, she::kKey0Pad},
    // {SDLK_1PAD, she::kKey1Pad},
    // {SDLK_2PAD, she::kKey2Pad},
    // {SDLK_3PAD, she::kKey3Pad},
    // {SDLK_4PAD, she::kKey4Pad},
    // {SDLK_5PAD, she::kKey5Pad},
    // {SDLK_6PAD, she::kKey6Pad},
    // {SDLK_7PAD, she::kKey7Pad},
    // {SDLK_8PAD, she::kKey8Pad},
    // {SDLK_9PAD, she::kKey9Pad},
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
    {SDL_KeyCode(1073742051), she::kKeyLWin},
    // {SDLK_RWIN, she::kKeyRWin},
    {SDLK_MENU, she::kKeyMenu},
    {SDLK_SCROLLLOCK, she::kKeyScrLock},
    {SDLK_NUMLOCKCLEAR, she::kKeyNumLock},
    {SDLK_CAPSLOCK, she::kKeyCapsLock},
  };

namespace she {
namespace sdl {
    bool isMaximized;
    bool isMinimized;
    extern std::unordered_map<int, SDL2Display*> windowIdToDisplay;
}

    class SDL2EventQueue : public EventQueue {
    public:
        static SDL2EventQueue g_queue;

        void getEvent(Event& event, bool) override {
            for (auto& entry : sdl::windowIdToDisplay) {
                entry.second->present();
            }
            SDL_Event sdlEvent;
            while (SDL_PollEvent(&sdlEvent)) {
                switch (sdlEvent.type){
                case SDL_WINDOWEVENT:
                    switch (sdlEvent.window.event) {
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
                        std::cout << "Resized" << std::endl;
                        auto display = sdl::windowIdToDisplay[sdlEvent.window.windowID];
                        display->setWidth(sdlEvent.window.data1);
                        display->setHeight(sdlEvent.window.data2);
                        event.setType(Event::ResizeDisplay);
                        event.setDisplay(display);
                        return;
                    }

                    default:
                        std::cout << "Unknown windowevent: " << (int) sdlEvent.window.event << std::endl;
                        continue;
                    }
                    continue;

                case SDL_MOUSEMOTION:
                    event.setType(Event::MouseMove);
                    event.setPosition({
                            sdlEvent.motion.x / unique_display->scale(),
                            sdlEvent.motion.y / unique_display->scale()
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
                    return;
                }

                case SDL_KEYDOWN:
                case SDL_KEYUP: {
                    auto it = keyCodeMapping.find((SDL_KeyCode) sdlEvent.key.keysym.sym);
                    event.setType(sdlEvent.type == SDL_KEYUP ? Event::KeyUp : Event::KeyDown);
                    if (it != keyCodeMapping.end()) {
                        event.setScancode(it->second);
                        std::cout << "scancode: " << sdlEvent.key.keysym.sym << std::endl;
                    } else {
                        std::cout << "Unknown scancode: " << sdlEvent.key.keysym.sym << std::endl;
                        event.setScancode(she::kKeyUnknown1);
                    }
                    return;
                }

                // MouseDoubleClick,

                // CloseDisplay,
                // ResizeDisplay,
                // DropFiles,
                // MouseEnter,
                // MouseLeave,
                // MouseWheel,
                // MouseDoubleClick,
                // KeyDown,
                // KeyUp,
                // TouchMagnify,
                case SDL_QUIT:
                    event.setType(Event::CloseDisplay);
                    return;

                case SDL_KEYMAPCHANGED:
                    continue;

                default:
                    std::cout << "Unknown event: " << sdlEvent.type << std::endl;
                    continue;
                }
            }
            event.setType(Event::None);
        }

        void queueEvent(const Event& event) override {
            m_events.push(event);
        }

    private:
        // We need a concurrent queue because events are generated in one
        // thread (the thread created by Allegro 4 for the HWND), and
        // consumed in the other thread (the main/program logic thread).
        base::concurrent_queue<Event> m_events;
    };

    SDL2EventQueue g_queue;

    EventQueue* EventQueue::instance() {
        return &g_queue;
    }

    class SDL2System : public CommonSystem {
    public:
        SDL2System() {
//     if (allegro_init() < 0)
//       throw base::Exception("Cannot initialize Allegro library: %s", allegro_error);

//     set_uformat(U_UTF8);
// #if MAKE_VERSION(ALLEGRO_VERSION, ALLEGRO_SUB_VERSION, ALLEGRO_WIP_VERSION) >= MAKE_VERSION(4, 4, 0)
//     _al_detect_filename_encoding();
// #endif
//     install_timer();

//     // Register PNG as a supported bitmap type
//     register_bitmap_file_type("png", load_png, save_png);

//     // Init event sources
//     display_events_init();
// #ifdef USE_KEY_POLLER
//     key_poller_init();
// #endif
// #ifdef USE_MOUSE_POLLER
//     mouse_poller_init();
// #endif

                g_instance = this;
            }

        ~SDL2System() {
            IMG_Quit();
            SDL_Quit();
            g_instance = nullptr;
        }

        void dispose() override {
            delete this;
        }

        void activateApp() override {
            // Do nothing
        }

        void finishLaunching() override {
            // Do nothing
        }

        Capabilities capabilities() const override {
            return (Capabilities)(Capabilities::CanResizeDisplay);
        }

        EventQueue* eventQueue() override { // TODO remove this function
            return EventQueue::instance();
        }

        bool gpuAcceleration() const override {
            return true;
        }

        void setGpuAcceleration(bool state) override {
            // Do nothing
        }

        gfx::Size defaultNewDisplaySize() override {
            return gfx::Size(0, 0);
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

        Surface* loadSurface(const char* filename) override {
            SDL_Surface* bmp = IMG_Load(filename);
            if (!bmp)
              throw std::runtime_error("Error loading image");

            std::cout << "Loading " << filename << " "
                      << std::to_string(bmp->format->BitsPerPixel) << " "
                      << std::to_string(bmp->format->Rshift) << " "
                      << std::to_string(bmp->format->Ashift) << std::endl;

            return new SDL2Surface(bmp, SDL2Surface::DeleteAndDestroy);
        }

        Surface* loadRgbaSurface(const char* filename) override {
            return loadSurface(filename);
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

#ifdef _WIN32
        std::wstring wmsg = base::from_utf8(msg);
        std::wstring title = base::from_utf8(PACKAGE);
        ::MessageBoxW(NULL, wmsg.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
#else
        printf("%s", msg);
#endif
    }

    bool is_key_pressed(KeyScancode scancode)
    {
// #ifdef ALLEGRO_UNIX
//   if (scancode == kKeyLShift || scancode == kKeyRShift) {
//     return key_shifts & KB_SHIFT_FLAG;
//   }
//   else if (scancode == kKeyLControl || scancode == kKeyRControl) {
//     return key_shifts & KB_CTRL_FLAG;
//   }
//   else if (scancode == kKeyAlt) {
//     return key_shifts & KB_ALT_FLAG;
//   }
// #endif
//   return key[scancode] ? true: false;
        return false;
    }

    void clear_keyboard_buffer()
    {
        // clear_keybuf();
    }

} // namespace she

// It must be defined by the user program code.
extern int app_main(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS) != 0) {
        std::cerr << "Critical: Could not initialize SDL2. Aborting." << std::endl;
        return -1;
    }
    if (!IMG_Init( IMG_INIT_PNG | IMG_INIT_WEBP | IMG_INIT_JPG )) {
        std::cerr << "Critical: Could not initialize SDL2_image. Aborting." << std::endl;
        return -2;
    }

    return app_main(argc, argv);
}
