module;
#include "core.h"
#include <cstdint>

export module game.intfc;

namespace game {
    export class IWindow
    {
    public:

        virtual uint32_t GetWidth() = 0;
        virtual uint32_t GetHeight() = 0;
        virtual HWND GetHandle() = 0;
        virtual LRESULT OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
    };
}
