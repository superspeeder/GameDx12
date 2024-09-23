module;
#include "pch.h"


export module game.window;
export import std;

import game;

namespace game {
    export struct WindowSettings {
    };

    export class Window {
        explicit Window(const WindowSettings& settings);

        HWND 
    public:

        static std::shared_ptr<Window> Create(const WindowSettings& settings) {
            return std::shared_ptr<Window>(new Window(settings));
        }

        void Show(int swParam) {

        }

    private:
    };

    Window::Window(const WindowSettings& settings) {
        
    }

}
