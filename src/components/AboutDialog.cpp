// AboutDialog.cpp
#include "components/AboutDialog.h"

#include <windows.h>
#include <iostream>

#include "win32/HashMapMessageHandler.h"

AboutDialog::AboutDialog(HINSTANCE hInstance, HWND parent)
    : hInstance_(hInstance), parent_(parent)
{
    InitializeMessageHandlers();
}

AboutDialog::~AboutDialog()
{
    AboutDialog::OnDestroy();
}

void AboutDialog::InitializeMessageHandlers()
{
    messageHandler_.RegisterHandler(WM_CREATE, [this](HWND,WPARAM, LPARAM) -> LRESULT
    {
        constexpr int dlgWidth = 360;
        constexpr int dlgHeight = 180;

        ::CreateWindowExW(0, L"STATIC", L"Sample App\nVersion 1.0\n(c) Example",
                          WS_CHILD | WS_VISIBLE | SS_CENTER,
                          10, 10, dlgWidth - 20, 90,
                          dlgWindow_, nullptr, hInstance_, nullptr);

        ::CreateWindowExW(0, L"BUTTON", L"OK",
                          WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                          (dlgWidth - 80) / 2, dlgHeight - 60, 80, 30,
                          dlgWindow_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_ABOUT_OK)), hInstance_, nullptr);

        return 0;
    });

    messageHandler_.RegisterHandler(WM_COMMAND, [this](HWND,WPARAM wParam, LPARAM) -> LRESULT
    {
        const int cmdId = static_cast<int>(LOWORD(wParam));
        if (cmdId == ID_ABOUT_OK)
        {
            ::DestroyWindow(dlgWindow_);
        }
        return 0;
    });

    messageHandler_.RegisterHandler(WM_DESTROY, [this](HWND, WPARAM, LPARAM) -> LRESULT
    {
        dlgWindow_ = nullptr;
        return 0;
    });
}

void AboutDialog::Show()
{
    CreateDialogWindow();
    if (dlgWindow_)
    {
        RunMessageLoop();
    }
}

void AboutDialog::OnCreate(HWND hwndParent) noexcept
{
}

void AboutDialog::OnDestroy() noexcept
{
    if (dlgWindow_)
    {
        ::DestroyWindow(dlgWindow_);
        dlgWindow_ = nullptr;
    }
}

bool AboutDialog::OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* outResult) noexcept
{
    return false;
}

void AboutDialog::CreateDialogWindow()
{
    constexpr int dlgWidth = 360;
    constexpr int dlgHeight = 180;

    const int screenW = ::GetSystemMetrics(SM_CXSCREEN);
    const int screenH = ::GetSystemMetrics(SM_CYSCREEN);

    const int x = (screenW - dlgWidth) / 2;
    const int y = (screenH - dlgHeight) / 2;

    const wchar_t *dlgClass = L"SimpleAboutClass";

    WNDCLASSEXW wc{};
    if (::GetClassInfoExW(hInstance_, dlgClass, &wc) == FALSE)
    {
        WNDCLASSEXW wcNew{};
        wcNew.cbSize = sizeof(WNDCLASSEXW);
        wcNew.style = CS_HREDRAW | CS_VREDRAW;
        wcNew.lpfnWndProc = AboutDialog::StaticDlgProc;
        wcNew.hInstance = hInstance_;
        wcNew.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        wcNew.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wcNew.lpszClassName = dlgClass;
        ::RegisterClassExW(&wcNew);
    }

    dlgWindow_ = ::CreateWindowExW(
        0,
        dlgClass,
        L"About",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        x, y, dlgWidth, dlgHeight,
        parent_,
        nullptr,
        hInstance_,
        this
    );
    
    if (dlgWindow_) {
        // Show the window after creating it
        ::ShowWindow(dlgWindow_, SW_SHOW);
        ::UpdateWindow(dlgWindow_);
    }
}

void AboutDialog::RunMessageLoop() const
{
    ::EnableWindow(parent_, FALSE);

    MSG msg{};
    while (::IsWindow(dlgWindow_) && ::GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        // Check if the message is for the dialog or its children
        if (msg.hwnd == dlgWindow_ || ::IsChild(dlgWindow_, msg.hwnd))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
        else
        {
            // Dispatch other messages normally
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
        
        // If the dialog window was destroyed, break out of the loop
        if (!::IsWindow(dlgWindow_)) {
            break;
        }
    }

    ::EnableWindow(parent_, TRUE);
    ::SetActiveWindow(parent_);
}

LRESULT CALLBACK AboutDialog::StaticDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    AboutDialog *self = nullptr;

    if (msg == WM_NCCREATE)
    {
        const auto *cs = reinterpret_cast<CREATESTRUCTW *>(lParam);
        self = static_cast<AboutDialog *>(cs->lpCreateParams);
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        // Set dlgWindow_ immediately
        self->dlgWindow_ = hwnd;
    }
    else
    {
        self = reinterpret_cast<AboutDialog *>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    return self ? self->DlgProc(hwnd, msg, wParam, lParam) : ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT AboutDialog::DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    const LRESULT result = messageHandler_.HandleMessage(dlgWindow_, msg, wParam, lParam);
    if (result != win32::HashMapMessageHandler::MSG_NOT_HANDLED)
    {
        return result;
    }

    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}