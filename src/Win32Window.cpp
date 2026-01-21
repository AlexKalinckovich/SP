// Win32Window.cpp
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>
#include <commdlg.h>
#include "win32/Win32Window.h"

#include <filesystem>

#include "components/AboutDialog.h"
#include "components/FontSelectorComponent.h"
#include "components/ProcessInfoViewer.h"
#include "meta_info/message_codes.h"
#include "utils/ErrorFormatter.h"
#include "utils/FileManager.h"

namespace win32
{
    Win32Window::Win32Window(HINSTANCE hInstance, std::wstring className, std::wstring windowTitle) noexcept
    : hInstance_(hInstance)
    , className_(std::move(className))
    , windowTitle_(std::move(windowTitle))
    , overlayWindow_(std::make_shared<OverlayWindow>(hInstance, className_ + L"_Overlay"))
    , excelLikeView_(std::make_shared<ExcelLikeView>(10,10))
    , idleMonitor_(std::make_shared<IdleMonitor>())
    {
        ProcessInfoViewer::Register(hInstance_);
        InitializeMessageHandlers();
        AddComponent(idleMonitor_);
        AddComponent(overlayWindow_);
        excelLikeView_->OnCreate(hwnd_);
        AddComponent(excelLikeView_);
    }


    bool Win32Window::Register() noexcept
    {
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

        if (::RegisterClassExW(&wc) == 0)
        {
            return false;
        }

        if (!overlayWindow_)
        {
            return false;
        }

        const bool res = overlayWindow_->RegisterClass();
        if (res)
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

        messageHandler_.RegisterHandler(WM_CLOSE, [this](HWND hwnd, WPARAM, LPARAM) -> LRESULT
        {
            // if (textEditor_->HasUnsavedChanges())
            // {
            //     if (!textEditor_->PromptSaveIfNeeded(hwnd))
            //     {
            //         return 0;
            //     }
            // }
            ::DestroyWindow(hwnd);
            return 0;
        });

        messageHandler_.RegisterHandler(WM_DESTROY, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            componentManager_.OnDestroy();
            ::PostQuitMessage(0);
            return 0;
        });

        messageHandler_.RegisterCommandHandler(ID_FILE_OPEN, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            excelLikeView_->HandleFileOpen();
            return 0;
        });

        messageHandler_.RegisterCommandHandler(ID_FILE_SAVE, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            return 0;
        });

        messageHandler_.RegisterCommandHandler(ID_FILE_EXIT, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            ::DestroyWindow(hwnd_);
            return 0;
        });

        messageHandler_.RegisterCommandHandler(ID_EDIT_CUT, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            return 0;
        });

        messageHandler_.RegisterCommandHandler(ID_EDIT_COPY, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            return 0;
        });

        messageHandler_.RegisterCommandHandler(ID_EDIT_PASTE, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            return 0;
        });

        messageHandler_.RegisterCommandHandler(ID_EDIT_UNDO, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            return 0;
        });

        messageHandler_.RegisterCommandHandler(ID_EDIT_REDO, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            return 0;
        });

        messageHandler_.RegisterCommandHandler(ID_EDIT_SELECT_ALL, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            return 0;
        });

        messageHandler_.RegisterCommandHandler(ID_HELP_ABOUT, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            ShowAboutDialog();
            return 0;
        });

        messageHandler_.RegisterCommandHandler(ID_FORMAT_FONT, [this](HWND, WPARAM, LPARAM) -> LRESULT
        {
            ShowFontDialog();
            return 0;
        });

        messageHandler_.RegisterCommandHandler(ID_PROCESS_INFO, [this](HWND hWnd, WPARAM, LPARAM) -> LRESULT
        {
            HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE);

            RECT rcParent;
            GetWindowRect(hWnd, &rcParent);
            constexpr int childWidth = 350;
            constexpr int childHeight = 400;
            const int x = rcParent.left + (rcParent.right - rcParent.left - childWidth) / 2;
            const int y = rcParent.top + (rcParent.bottom - rcParent.top - childHeight) / 2;
            ProcessInfoViewer dialog(hInstance, hWnd);
            dialog.ShowModal(x, y, childWidth, childHeight);

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

    void Win32Window::AddComponent(std::shared_ptr<ui::IComponent> c) noexcept
    {
        componentManager_.AddComponent(std::move(c));
    }

    bool Win32Window::RemoveComponent(const ui::IComponent* cptr) noexcept
    {
        return componentManager_.RemoveComponent(cptr);
    }

    LRESULT Win32Window::HandleMessage(const UINT msg, const WPARAM wParam, const LPARAM lParam) noexcept
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


    LRESULT CALLBACK Win32Window::StaticWndProc(HWND hwnd,
                                                const UINT msg,
                                                const WPARAM wParam,
                                                const LPARAM lParam) noexcept
    {
        Win32Window* self = nullptr;

        if (msg == WM_NCCREATE)
        {
            const CREATESTRUCTW *cs = reinterpret_cast<CREATESTRUCTW *>(lParam);
            self = static_cast<Win32Window*>(cs->lpCreateParams);
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->hwnd_ = hwnd;
        }
        else
        {
            self = reinterpret_cast<Win32Window*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        LRESULT result;
        if (self)
        {
            result = self->HandleMessage(msg, wParam, lParam);
        }
        else
        {
            result = ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        return result;
    }
    void Win32Window::ShowAboutDialog() const
    {
        AboutDialog about(hInstance_, hwnd_);
        about.Show();
    }

    void Win32Window::ShowFontDialog() const
    {
        const std::filesystem::path fontDirectory = LR"(C:\Users\brota\CLionProjects\SP\meta-info)";

        FontSelectorDialog dialog(hInstance_, hwnd_, fontDirectory);
        const std::optional<std::wstring> selectedFontName = dialog.ShowModal();

        if (selectedFontName.has_value())
        {
            const std::wstring fontPath = fontDirectory / (selectedFontName.value() + L".ttf");
            const std::wstring& fontName = selectedFontName.value();

            this->excelLikeView_->SetFont(fontName, 36);
        }
    }


} // namespace win32
