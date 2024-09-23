#include "core.h"


import game.core;
import game.window;

int Run(int nCmdShow)
{
    auto window = game::Window::Create(game::WindowSettings{L"Window!", CW_USEDEFAULT, CW_USEDEFAULT, 800, 600});
    auto window2 = game::Window::Create(game::WindowSettings{L"Window 2!", CW_USEDEFAULT, CW_USEDEFAULT, 800, 600});

    window->Show(nCmdShow);
    window2->Show(nCmdShow);


    return game::EventLoop();
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hInstancePrev, PWSTR pCmdLine, int nCmdShow)
{
    game::Init(hInstance);

    int exitCode = Run(nCmdShow);

    game::Terminate();
    return exitCode;
}
