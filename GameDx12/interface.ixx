module;
#include "core.h"
#include <cstdint>

export module game.intfc;

namespace game {
    export class I2DSurface
    {
    public:

        virtual uint32_t GetWidth() = 0;
        virtual uint32_t GetHeight() = 0;
        virtual HWND GetHandle() = 0;
    };
}
