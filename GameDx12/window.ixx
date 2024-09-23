module;

#include "core.h"

export module game.window;

export import std;
export import game.intfc;

namespace game
{
    export struct WindowSettings
    {
        std::wstring title;
        int x = CW_USEDEFAULT;
        int y = CW_USEDEFAULT;
        int width = CW_USEDEFAULT;
        int height = CW_USEDEFAULT;
    };

    export class Window : public IWindow
    {
        explicit Window(const WindowSettings& settings);

        HWND m_Handle;

    public:
        virtual ~Window() = default;

        static std::shared_ptr<Window> Create(const WindowSettings& settings)
        {
            return std::shared_ptr<Window>(new Window(settings));
        }

        void Show(int swParam)
        {
            ShowWindow(m_Handle, swParam);
        }

        uint32_t GetWidth() override
        {
            RECT cr;
            GetClientRect(m_Handle, &cr);
            return cr.right - cr.left;
        }

        uint32_t GetHeight() override
        {
            RECT cr;
            GetClientRect(m_Handle, &cr);
            return cr.bottom - cr.top;
        }

        HWND GetHandle() override
        {
            return m_Handle;
        }

    private:

        std::function<void()> m_OnRender = +[]{};

    public:

        void SetOnRender(const std::function<void()>& f) {
            m_OnRender = f;
        }

        LRESULT OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override {
            if (msg == WM_PAINT)
            {
                m_OnRender();
                return 0;
            }

            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
    };
}
