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

        std::wstring assetsPath;
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
        } else
        {
            LONG_PTR lp = GetWindowLongPtrW(hWnd, GWLP_USERDATA);
            if (lp != 0)
            {
                auto *win = reinterpret_cast<IWindow*>(lp);
                return win->OnMessage(hWnd, uMsg, wParam, lParam);
            }
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

    inline void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize)
    {
        if (path == nullptr)
        {
            throw std::exception();
        }

        DWORD size = GetModuleFileName(nullptr, path, pathSize);
        if (size == 0 || size == pathSize)
        {
            // Method failed or path was truncated.
            throw std::exception();
        }

        WCHAR* lastSlash = wcsrchr(path, L'\\');
        if (lastSlash)
        {
            *(lastSlash + 1) = L'\0';
        }
    }

    export void Init(const HINSTANCE hInstance)
    {
        global = new Global;
        global->hInstance = hInstance;

        RegisterWC(hInstance);

        wchar_t ap[MAX_PATH];
        GetAssetsPath(ap, MAX_PATH);
        global->assetsPath = ap;
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

    export int EventLoop(const std::function<void()>& update)
    {
        MSG msg{};
        while (true)
        {
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE) != 0)
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);

                if (msg.message == WM_QUIT)
                {
                    return msg.wParam;
                }
            }

            update();
        }
    }

    export std::wstring GetAssetFullPath(const std::wstring& assetPath) {
        return global->assetsPath + assetPath;
    }
}
