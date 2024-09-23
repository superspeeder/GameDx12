module;

#include "core.h"

export module game.core;

export import std;
export import game.intfc;


namespace game
{
    struct Global
    {
        HINSTANCE hInstance;
        ATOM windowClass;

        std::atomic_int windowCounter = 0;
        std::atomic_int exitCode = 0;
        std::atomic_bool exitOnAllWindowsClosed = true;
    };

    Global* global;


    LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_CREATE)
        {
            ++global->windowCounter;
            auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            if (cs->lpCreateParams)
            {
                SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            }
            return 0;
        }
        else if (uMsg == WM_CLOSE)
        {
            // todo: optional closing

            DestroyWindow(hWnd);
            return 0;
        }
        else if (uMsg == WM_DESTROY)
        {
            --global->windowCounter;

            if (global->windowCounter == 0 && global->exitOnAllWindowsClosed)
            {
                PostQuitMessage(global->exitCode);
            }
            return 0;
        }

        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    export const auto WC_NAME = L"GameWC_B746E141-79A3-4F17-95BF-9BD556A5D421";

    void RegisterWC(const HINSTANCE hInstance)
    {
        WNDCLASSEXW wc{};
        wc.hInstance = hInstance;
        wc.cbSize = sizeof(wc);
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BACKGROUND + 1);
        wc.lpfnWndProc = WindowProc;
        wc.lpszClassName = WC_NAME;
        wc.style = CS_VREDRAW | CS_HREDRAW;

        global->windowClass = RegisterClassExW(&wc);
    }

    export void Init(const HINSTANCE hInstance)
    {
        global = new Global;
        global->hInstance = hInstance;

        RegisterWC(hInstance);
    }

    export void Terminate()
    {
        if (global)
        {
            UnregisterClassW(reinterpret_cast<LPCWSTR>(global->windowClass), global->hInstance);
        }

        delete global;
        global = nullptr;
    }

    export HINSTANCE GetInstanceHandle()
    {
        return global->hInstance;
    }

    export int EventLoop()
    {
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
}
