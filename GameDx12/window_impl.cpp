#include "core.h"

import game.core;
import game.window;

namespace game
{

    Window::Window(const WindowSettings& settings)
    {
        m_Handle = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW, WC_NAME, settings.title.c_str(), WS_OVERLAPPEDWINDOW,
            settings.x, settings.y, settings.width, settings.height, nullptr, nullptr,
            GetInstanceHandle(), this);
    }
}
