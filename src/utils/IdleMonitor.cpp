// IdleMonitor.cpp
#include "utils/IdleMonitor.h"

#include <windows.h>
#include <iostream>

#include "components/OverlayWindow.h"


IdleMonitor::IdleMonitor()
{
    std::cout << "IdleMonitor constructor" << '\n';
    InitializeMessageHandlers();
}

IdleMonitor::~IdleMonitor()
{
    IdleMonitor::OnDestroy();
}

void IdleMonitor::InitializeMessageHandlers()
{
    messageHandler_.RegisterHandler(WM_SIZE, [this](HWND, const WPARAM wParam, const LPARAM lParam) -> LRESULT
    {
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

    messageHandler_.RegisterHandler(WM_ACTIVATE, [this](HWND, const WPARAM wParam, LPARAM) -> LRESULT
    {
        if(hwndParent_)
        {
            const WORD activationType = LOWORD(wParam);
            const BOOL minimized = HIWORD(wParam);

            std::cout << "WM_ACTIVATE - activationType: " << activationType
                      << ", minimized: " << minimized << std::endl;

            if (activationType == WA_INACTIVE)
            {
                if (!isPaused_)
                {
                    std::cout << "Window deactivated - pausing idle monitor" << std::endl;
                    this->Pause();
                }
            }
            else
            {
                if (isPaused_)
                {
                    std::cout << "Window activated - resuming idle monitor" << std::endl;
                    this->Resume();
                }
            }
        }
        return 0;
    });
    messageHandler_.RegisterHandler(WM_TIMER,[this](HWND, const WPARAM wParam,LPARAM) -> LRESULT
    {
        if(wParam == IDLE_TIMER_ID)
        {
            const ULONGLONG idle = GetIdleTimeMs();
            std::cout << "Idle time: " << idle << " ms, Threshold: " << IDLE_THRESHOLD_MS << " ms" << std::endl;

            if (idle >= IDLE_THRESHOLD_MS)
            {
                std::cout << "Idle threshold reached, sending WM_IDLE_TIMEOUT" << std::endl;
                ::PostMessageW(hwndParent_, WM_IDLE_TIMEOUT, 0, 0);
            }
            else
            {
                std::cout << "Idle threshold NOT reached yet" << std::endl;
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

bool IdleMonitor::OnMessage(HWND hwnd, const UINT msg, const WPARAM wParam, const LPARAM lParam, LRESULT* outResult) noexcept {
    *outResult = messageHandler_.HandleMessage(hwnd, msg, wParam, lParam);
    const bool handled = (*outResult != win32::HashMapMessageHandler::MSG_NOT_HANDLED);
    return handled;
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
        std::cout << "Killing timer ID: " << IDLE_TIMER_ID << std::endl;
        ::KillTimer(hwndParent_, IDLE_TIMER_ID);
        isPaused_ = true;
        std::cout << "Idle monitor paused" << std::endl;
    }
    else
    {
        std::cout << "Pause called but already paused or no hwndParent" << std::endl;
    }
}

void IdleMonitor::Resume() noexcept
{
    if (hwndParent_ && isPaused_)
    {
        std::cout << "Setting timer ID: " << IDLE_TIMER_ID << " with hwnd: " << hwndParent_ << std::endl;
        ::SetTimer(hwndParent_, IDLE_TIMER_ID, 1000, nullptr);
        isPaused_ = false;
        std::cout << "Idle monitor resumed" << std::endl;
    }
    else
    {
        std::cout << "Resume called but not paused or no hwndParent. isPaused_: " << isPaused_
                  << ", hwndParent_: " << hwndParent_ << std::endl;
    }
}
