#include "components/OverlayWindow.h"

#include <iostream>

#include "utils/IdleMonitor.h"

OverlayWindow::OverlayWindow(HINSTANCE hInstance, std::wstring className)
    : hInstance_(hInstance), className_(std::move(className)) {
    InitializeMessageHandlers();
}

OverlayWindow::~OverlayWindow() {
    Destroy();
}

bool OverlayWindow::RegisterClass()
{
    WNDCLASSEXW wc{};
    if (::GetClassInfoExW(hInstance_, className_.c_str(), &wc)) {
        classRegistered_ = true;
        return true;
    }

    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &OverlayWindow::StaticWndProc;
    wc.hInstance = hInstance_;
    wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = className_.c_str();

    classRegistered_ = (::RegisterClassExW(&wc) != 0);
    return classRegistered_;
}

void OverlayWindow::OnCreate(HWND hwndParent) noexcept {
    parentHwnd_ = hwndParent;
}

void OverlayWindow::OnDestroy() noexcept {
    Destroy();
}

bool OverlayWindow::OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* outResult) noexcept {
    if(msg != WM_IDLE_TIMEOUT)
    {
        if (!isVisible_ || !hwnd_) {
            return false;
        }
    }

    *outResult = messageHandler_.HandleMessage(hwnd, msg, wParam, lParam);
    return *outResult != win32::HashMapMessageHandler::MSG_NOT_HANDLED;
}

void OverlayWindow::InitializeMessageHandlers() {
    messageHandler_.RegisterHandler(WM_IDLE_TIMEOUT, [this](HWND hwnd, WPARAM, LPARAM) -> LRESULT {
            if (!isVisible_) {
                Create();
            }
            return 0;
        });

    messageHandler_.RegisterHandler(WM_CREATE, [this](HWND hwnd, WPARAM, LPARAM) -> LRESULT {
        spriteState_ = new SpriteState;

        RECT clientRect;
        ::GetClientRect(hwnd, &clientRect);
        const int width = clientRect.right - clientRect.left;
        const int height = clientRect.bottom - clientRect.top;

        spriteState_->w = 48;
        spriteState_->h = 48;
        spriteState_->x = width / 2 - spriteState_->w / 2;
        spriteState_->y = height / 2 - spriteState_->h / 2;
        spriteState_->dx = 4;
        spriteState_->dy = 3;

        ::SetTimer(hwnd, 2002, 500, nullptr);
        return 0;
    });

    messageHandler_.RegisterHandler(WM_TIMER, [this](HWND hwnd, WPARAM wParam, LPARAM) -> LRESULT {
        if (wParam == OVERLAY_ANIM_TIMER && spriteState_) {
            RECT clientRect;
            ::GetClientRect(hwnd, &clientRect);
            const int width = clientRect.right - clientRect.left;
            const int height = clientRect.bottom - clientRect.top;

            spriteState_->x += spriteState_->dx;
            spriteState_->y += spriteState_->dy;

            if (spriteState_->x < 0) {
                spriteState_->x = 0;
                spriteState_->dx = -spriteState_->dx;
            } else if (spriteState_->x + spriteState_->w > width) {
                spriteState_->x = width - spriteState_->w;
                spriteState_->dx = -spriteState_->dx;
            }

            if (spriteState_->y < 0) {
                spriteState_->y = 0;
                spriteState_->dy = -spriteState_->dy;
            } else if (spriteState_->y + spriteState_->h > height) {
                spriteState_->y = height - spriteState_->h;
                spriteState_->dy = -spriteState_->dy;
            }

            ::InvalidateRect(hwnd, nullptr, TRUE);
        } else if (wParam == 2002) {
            ignoreFirstInput_ = false;
            ::KillTimer(hwnd, 2002);
        }
        return 0;
    });

    messageHandler_.RegisterHandler(WM_PAINT, [this](HWND hwnd, WPARAM, LPARAM) -> LRESULT {
        PAINTSTRUCT ps;
        HDC hdc = ::BeginPaint(hwnd, &ps);

        RECT client;
        ::GetClientRect(hwnd, &client);
        HBRUSH blackBrush = ::CreateSolidBrush(RGB(0, 0, 0));
        ::FillRect(hdc, &client, blackBrush);
        ::DeleteObject(blackBrush);

        if (spriteState_) {
            const RECT r{spriteState_->x, spriteState_->y,
                   spriteState_->x + spriteState_->w, spriteState_->y + spriteState_->h};
            HBRUSH redBrush = ::CreateSolidBrush(RGB(200, 40, 40));
            ::FillRect(hdc, &r, redBrush);
            ::DeleteObject(redBrush);
        }

        ::EndPaint(hwnd, &ps);
        return 0;
    });

    messageHandler_.RegisterHandler(WM_ERASEBKGND, [](HWND, WPARAM, LPARAM) -> LRESULT {
        return 1;
    });

    messageHandler_.RegisterHandler(WM_DESTROY, [this](HWND hwnd, WPARAM, LPARAM) -> LRESULT {
        ::KillTimer(hwnd, OVERLAY_ANIM_TIMER);
        ::KillTimer(hwnd, 2002);
        delete spriteState_;
        spriteState_ = nullptr;
        hwnd_ = nullptr;
        isVisible_ = false;
        return 0;
    });

    const auto inputHandler = [this](HWND hwnd, WPARAM, LPARAM) -> LRESULT {
        if(!ignoreFirstInput_)
        {
            Destroy();
        }
        return 0;
    };

    messageHandler_.RegisterHandler(WM_MOUSEMOVE, inputHandler);
    messageHandler_.RegisterHandler(WM_LBUTTONDOWN, inputHandler);
    messageHandler_.RegisterHandler(WM_RBUTTONDOWN, inputHandler);
    messageHandler_.RegisterHandler(WM_MBUTTONDOWN, inputHandler);
    messageHandler_.RegisterHandler(WM_KEYDOWN, inputHandler);
    messageHandler_.RegisterHandler(WM_SYSKEYDOWN, inputHandler);
    messageHandler_.RegisterHandler(WM_SIZE, inputHandler);
    messageHandler_.RegisterHandler(WM_CANCELMODE, inputHandler);

}

bool OverlayWindow::Create()
{
    if (!parentHwnd_)
    {
        return false;
    }

    RECT clientRect;
    ::GetClientRect(parentHwnd_, &clientRect);

    POINT topLeft = {0, 0};
    POINT bottomRight = {clientRect.right, clientRect.bottom};
    ::ClientToScreen(parentHwnd_, &topLeft);
    ::ClientToScreen(parentHwnd_, &bottomRight);

    const int width = bottomRight.x - topLeft.x;
    const int height = bottomRight.y - topLeft.y;

    hwnd_ = ::CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT,
        className_.c_str(),
        nullptr,
        WS_POPUP,
        topLeft.x, topLeft.y, width, height,
        parentHwnd_,
        nullptr,
        hInstance_,
        this
    );

    if (!hwnd_)
    {
        const DWORD err = GetLastError();
        std::cout << "Failed to create overlay window " << err << std::endl;
        return false;
    }

    ::SetLayeredWindowAttributes(hwnd_, 0, 128, LWA_ALPHA);

    ::ShowWindow(hwnd_, SW_SHOW);
    ::UpdateWindow(hwnd_);

    ignoreFirstInput_ = true;
    isVisible_ = true;

    ::SetTimer(hwnd_, OVERLAY_ANIM_TIMER, 33, nullptr);

    std::cout << "Overlay window created: " << hwnd_ << std::endl;
    return true;
}

void OverlayWindow::Destroy() {
    if (hwnd_) {
        ::KillTimer(hwnd_, OVERLAY_ANIM_TIMER);
        ::DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    isVisible_ = false;
}

LRESULT CALLBACK OverlayWindow::StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    OverlayWindow* self = nullptr;

    if (msg == WM_NCCREATE) {
        const auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<OverlayWindow*>(cs->lpCreateParams);
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    } else {
        self = reinterpret_cast<OverlayWindow*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    return self ? self->HandleMessage(hwnd, msg, wParam, lParam) : ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT OverlayWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    const LRESULT result = messageHandler_.HandleMessage(hwnd, msg, wParam, lParam);
    if (result != win32::HashMapMessageHandler::MSG_NOT_HANDLED) {
        return result;
    }

    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}