#include "core.h"

#include <io.h>
#include <fcntl.h>

import game.core;
import game.window;
import game.graphics;

int Run(int nCmdShow) {
    auto window = game::Window::Create(game::WindowSettings{L"Window!", CW_USEDEFAULT, CW_USEDEFAULT, 800, 600});

    window->Show(nCmdShow);

    auto renderSystem = game::RenderSystem::Create(window);

    window->SetOnRender([&]() {
        renderSystem->OnRender();
    });


    return game::EventLoop([&]() {
        renderSystem->OnUpdate();
        renderSystem->OnRender();
    });
}

void OpenConsole() {


    AllocConsole();
    std::wstring strW = L"Dev Console";
    SetConsoleTitle(strW.c_str());

    EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_GRAYED);
    DrawMenuBar(GetConsoleWindow());

    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &consoleInfo);

    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();

    HANDLE hConOut = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE hConIn = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
    SetStdHandle(STD_ERROR_HANDLE, hConOut);
    SetStdHandle(STD_INPUT_HANDLE, hConIn);
    std::wcout.clear();
    std::wclog.clear();
    std::wcerr.clear();
    std::wcin.clear();
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hInstancePrev, PWSTR pCmdLine, int nCmdShow) {
    game::Init(hInstance);
    OpenConsole();

    int exitCode = Run(nCmdShow);

    game::Terminate();

    EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_ENABLED);
    DrawMenuBar(GetConsoleWindow());

    std::cout << "\nPress Enter To Exit...";
    std::cout << getc(stdin);

    return exitCode;
}
