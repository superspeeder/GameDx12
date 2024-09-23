#include "pch.h"


import game;

int Run(int nCmdShow)
{
    HWND hwnd = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW, game::WC_NAME, L"Window!", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                CW_USEDEFAULT, 800, 600, nullptr, nullptr, game::GetInstanceHandle(), nullptr);

    ShowWindow(hwnd, nCmdShow);

    HWND hwnd2 = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW, game::WC_NAME, L"Window 2!", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
        CW_USEDEFAULT, 800, 600, nullptr, nullptr, game::GetInstanceHandle(), nullptr);

    ShowWindow(hwnd2, nCmdShow);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) != 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (msg.message == WM_QUIT)
    {
        return msg.wParam;
    }

    return 0;
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hInstancePrev, PWSTR pCmdLine, int nCmdShow)
{
    game::Init(hInstance);

    int exitCode = Run(nCmdShow);

    game::Terminate();
    return exitCode;
}
