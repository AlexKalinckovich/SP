// IdleMonitor.cpp
#include "utils/IdleMonitor.h"

#include <windows.h>
#include <iostream>

#include "components/OverlayWindow.h"


IdleMonitor::IdleMonitor()
{
    InitializeMessageHandlers();
}

IdleMonitor::~IdleMonitor()
{
    IdleMonitor::OnDestroy();
}

void IdleMonitor::InitializeMessageHandlers()
{
    messageHandler_.RegisterHandler(WM_SIZE, [this](HWND hwnd, WPARAM wParam, LPARAM lParam) -> LRESULT
    {
        LOWORD(lParam);
        HIWORD(lParam);

        if(hwndParent_)
        {
            switch (wParam)
            {
                case SIZE_MINIMIZED:
                    std::cout << "Window minimized - pausing idle monitor" << std::endl;
                    this->Pause();
                    break;

                case SIZE_RESTORED:
                    std::cout << "Window restored - resuming idle monitor" << std::endl;
                    this->Resume();
                    break;

                case SIZE_MAXIMIZED:
                    std::cout << "Window maximized" << std::endl;
                    break;
                default:
                    return 0;
            }
        }
        return 0;
    });

    messageHandler_.RegisterHandler(WM_ACTIVATE, [this](HWND hwnd, WPARAM wParam, LPARAM lParam) -> LRESULT
    {
        if(hwndParent_)
        {
            if (LOWORD(wParam) == WA_INACTIVE)
            {
                std::cout << "Window deactivated - pausing idle monitor" << std::endl;
                this->Pause();
            }
            else
            {
                std::cout << "Window activated - resuming idle monitor" << std::endl;
                this->Resume();
            }
        }
        return 0;
    });
    messageHandler_.RegisterHandler(WM_TIMER,[this](HWND,WPARAM wParam,LPARAM) -> LRESULT
    {
        if(wParam == IDLE_TIMER_ID)
        {
            const ULONGLONG idle = GetIdleTimeMs();
            std::cout << "Idle time: " << idle << " ms" << std::endl;

            if (idle >= IDLE_THRESHOLD_MS)
            {
                std::cout << "Idle threshold reached, sending WM_IDLE_TIMEOUT" << std::endl;
                ::PostMessageW(hwndParent_, WM_IDLE_TIMEOUT, 0, 0);
            }
        }
        return 0;
    });

    messageHandler_.RegisterHandler(WM_OVERLAY_WINDOW_DESTROY,[this](HWND, WPARAM, LPARAM) -> LRESULT
    {
        this->Resume();
        return 0;
    });

}

void IdleMonitor::OnCreate(HWND hwndParent) noexcept
{
    hwndParent_ = hwndParent;
    ::SetTimer(hwndParent_, IDLE_TIMER_ID, 1000, nullptr);
    std::cout << "Idle monitor started" << std::endl;
}

void IdleMonitor::OnDestroy() noexcept {
    if (hwndParent_)
    {
        ::KillTimer(hwndParent_, IDLE_TIMER_ID);
        hwndParent_ = nullptr;
        std::cout << "Idle monitor stopped" << std::endl;
    }
}

bool IdleMonitor::OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* outResult) noexcept {
    *outResult = messageHandler_.HandleMessage(hwnd, msg, wParam, lParam);
    return *outResult != win32::HashMapMessageHandler::MSG_NOT_HANDLED;
}

ULONGLONG IdleMonitor::GetIdleTimeMs() noexcept
{
    LASTINPUTINFO li{};
    li.cbSize = sizeof(li);
    if (::GetLastInputInfo(&li) == FALSE)
    {
        return 0;
    }

    const ULONGLONG now = ::GetTickCount64();
    const ULONGLONG last = li.dwTime;
    if (now >= last)
    {
        return now - last;
    }

    return 0;
}

void IdleMonitor::Pause() noexcept
{
    if (hwndParent_ && !isPaused_)
    {
        ::KillTimer(hwndParent_, IDLE_TIMER_ID);
        isPaused_ = true;
        std::cout << "Idle monitor paused" << std::endl;
    }
}

void IdleMonitor::Resume() noexcept
{
    if (hwndParent_ && isPaused_)
    {
        ::SetTimer(hwndParent_, IDLE_TIMER_ID, 1000, nullptr);
        isPaused_ = false;
        std::cout << "Idle monitor resumed" << std::endl;
    }
}
