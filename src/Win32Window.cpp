// Win32Window.cpp
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>
#include "win32/Win32Window.h"

#include "components/AboutDialog.h"

namespace win32
{
    Win32Window::Win32Window(HINSTANCE hInstance, std::wstring className, std::wstring windowTitle) noexcept
    : hInstance_(hInstance)
    , className_(std::move(className))
    , windowTitle_(std::move(windowTitle))
    , overlayWindow_(std::make_shared<OverlayWindow>(hInstance, className_ + L"_Overlay"))
    {
        idleMonitor_ = std::make_shared<IdleMonitor>();

        InitializeMessageHandlers();
        componentManager_.AddComponent(idleMonitor_);
        componentManager_.AddComponent(overlayWindow_);
    }

    bool Win32Window::Register() noexcept {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = &Win32Window::StaticWndProc;
        wc.hInstance = hInstance_;
        wc.hIcon = ::LoadIconW(nullptr, IDI_APPLICATION);
        wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = className_.c_str();
        wc.hIconSm = ::LoadIconW(nullptr, IDI_APPLICATION);

        if (::RegisterClassExW(&wc) == 0) {
            return false;
        }

        if (!overlayWindow_) {
            return false;
        }

        const bool res = overlayWindow_->RegisterClass();
        if(res)
        {
            AddComponent(overlayWindow_);
        }
        return res;
    }

    void Win32Window::InitializeMessageHandlers()
    {
        messageHandler_.RegisterHandler(WM_CREATE, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            componentManager_.OnCreate(hwnd_);
            return 0;
        });

        messageHandler_.RegisterHandler(WM_DESTROY, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            componentManager_.OnDestroy();
            ::PostQuitMessage(0);
            return 0;
        });

        messageHandler_.RegisterCommandHandler(MenuBar::ID_FILE_OPEN, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            ::MessageBoxW(hwnd_, L"Open selected (stub)", L"File", MB_OK);
            return 0;
        });

        messageHandler_.RegisterCommandHandler(MenuBar::ID_FILE_SAVE, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            ::MessageBoxW(hwnd_, L"Save selected (stub)", L"File", MB_OK);
            return 0;
        });

        messageHandler_.RegisterCommandHandler(MenuBar::ID_FILE_EXIT, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            ::DestroyWindow(hwnd_);
            return 0;
        });

        messageHandler_.RegisterCommandHandler(MenuBar::ID_EDIT_CUT, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            HWND focus = ::GetFocus();
            if (focus)
            {
                ::SendMessageW(focus, WM_CUT, 0, 0);
            }
            return 0;
        });

        messageHandler_.RegisterCommandHandler(MenuBar::ID_EDIT_COPY, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            HWND focus = ::GetFocus();
            if (focus)
            {
                ::SendMessageW(focus, WM_COPY, 0, 0);
            }
            return 0;
        });

        messageHandler_.RegisterCommandHandler(MenuBar::ID_EDIT_PASTE, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            HWND focus = ::GetFocus();
            if (focus)
            {
                ::SendMessageW(focus, WM_PASTE, 0, 0);
            }
            return 0;
        });

        messageHandler_.RegisterCommandHandler(MenuBar::ID_HELP_ABOUT, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            ShowAboutDialog();
            return 0;
        });

        messageHandler_.RegisterHandler(WM_SIZE, [this](HWND hwnd, WPARAM wParam, LPARAM lParam) -> LRESULT
        {
            LOWORD(lParam);
            HIWORD(lParam);

            switch (wParam)
            {
                case SIZE_MINIMIZED:
                    std::cout << "Window minimized - pausing idle monitor" << std::endl;
                    if (idleMonitor_)
                    {
                        idleMonitor_->Pause();
                    }
                    if (overlayWindow_ && overlayWindow_->IsVisible())
                    {
                        overlayWindow_->Destroy();
                    }
                    break;

                case SIZE_RESTORED:
                    std::cout << "Window restored - resuming idle monitor" << std::endl;
                    if (idleMonitor_)
                    {
                        idleMonitor_->Resume();
                    }
                    break;

                case SIZE_MAXIMIZED:
                    std::cout << "Window maximized" << std::endl;
                    break;
                default: return 0;
            }

            return 0;
        });

        messageHandler_.RegisterHandler(WM_ACTIVATE, [this](HWND hwnd, WPARAM wParam, LPARAM lParam) -> LRESULT
        {
            if (LOWORD(wParam) == WA_INACTIVE)
            {
                std::cout << "Window deactivated - pausing idle monitor" << std::endl;
                if (idleMonitor_)
                {
                    idleMonitor_->Pause();
                }
                if (overlayWindow_ && overlayWindow_->IsVisible())
                {
                    overlayWindow_->Destroy();
                }
            }
            else
            {
                std::cout << "Window activated - resuming idle monitor" << std::endl;
                if (idleMonitor_)
                {
                    idleMonitor_->Resume();
                }
            }
            return 0;
        });
    }

    Win32Window::~Win32Window() noexcept
    {
        if (hwnd_)
        {
            ::DestroyWindow(hwnd_);
            hwnd_ = nullptr;
        }
    }

    bool Win32Window::Create(const int x, const int y, const int width, const int height,
                             const DWORD exStyle, const DWORD style) noexcept
    {
        if (className_.empty())
        {
            return false;
        }

        hwnd_ = ::CreateWindowExW(
            exStyle,
            className_.c_str(),
            windowTitle_.c_str(),
            style,
            x, y, width, height,
            nullptr,
            nullptr,
            hInstance_,
            this
        );

        if (hwnd_)
        {
            if (menuBar_.Create())
            {
                ::SetMenu(hwnd_, menuBar_.GetHandle());
            }

            AddComponent(idleMonitor_);
        }

        return hwnd_ != nullptr;
    }

    void Win32Window::Show(const int nCmdShow) const noexcept
    {
        if (hwnd_)
        {
            ::ShowWindow(hwnd_, nCmdShow);
        }
    }

    void Win32Window::Update() const noexcept
    {
        if (hwnd_)
        {
            ::UpdateWindow(hwnd_);
        }
    }

    int Win32Window::ShowAndRun(const int nCmdShow) const noexcept
    {
        if (!hwnd_)
        {
            return -1;
        }

        Show(nCmdShow);
        Update();

        MSG msg{};
        while (::GetMessageW(&msg, nullptr, 0, 0) > 0)
        {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }

        return static_cast<int>(msg.wParam);
    }

    void Win32Window::AddComponent(std::shared_ptr<ui::IComponent> c) noexcept {
        componentManager_.AddComponent(c);
    }

    bool Win32Window::RemoveComponent(const ui::IComponent* cptr) noexcept {
        return componentManager_.RemoveComponent(cptr);
    }

    LRESULT Win32Window::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) noexcept
    {
        LRESULT outResult = 0;
        componentManager_.OnMessage(hwnd_, msg, wParam, lParam, &outResult);

        const LRESULT result = messageHandler_.HandleMessage(hwnd_, msg, wParam, lParam);
        if (result != HashMapMessageHandler::MSG_NOT_HANDLED)
        {
            return result;
        }

        return ::DefWindowProcW(hwnd_, msg, wParam, lParam);
    }


LRESULT CALLBACK Win32Window::StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
    Win32Window* self = nullptr;

    if (msg == WM_NCCREATE)
    {
        const auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<Win32Window*>(cs->lpCreateParams);
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    }
    else
    {
        self = reinterpret_cast<Win32Window*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    return self ? self->HandleMessage(msg, wParam, lParam) : ::DefWindowProcW(hwnd, msg, wParam, lParam);
}
void Win32Window::ShowAboutDialog() const
{
    AboutDialog about(hInstance_, hwnd_);
    about.Show();
}

} // namespace win32
