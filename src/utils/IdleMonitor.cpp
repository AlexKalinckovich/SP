// IdleMonitor.cpp
#include "utils/IdleMonitor.h"

#include <windows.h>
#include <iostream>

IdleMonitor::IdleMonitor() = default;

IdleMonitor::~IdleMonitor()
{
    IdleMonitor::OnDestroy();
}

void IdleMonitor::OnCreate(HWND hwndParent) noexcept
{
    hwndParent_ = hwndParent;
    ::SetTimer(hwndParent_, IDLE_TIMER_ID, 1000, nullptr);
    std::cout << "Idle monitor started" << std::endl;
}

void IdleMonitor::OnDestroy() noexcept {
    if (hwndParent_) {
        ::KillTimer(hwndParent_, IDLE_TIMER_ID);
        hwndParent_ = nullptr;
        std::cout << "Idle monitor stopped" << std::endl;
    }
}

bool IdleMonitor::OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* outResult) noexcept {
    if (msg == WM_TIMER && wParam == IDLE_TIMER_ID) {
        ULONGLONG idle = GetIdleTimeMs();
        std::cout << "Idle time: " << idle << " ms" << std::endl;

        if (idle >= IDLE_THRESHOLD_MS) {
            std::cout << "Idle threshold reached, sending WM_IDLE_TIMEOUT" << std::endl;
            ::PostMessageW(hwndParent_, WM_IDLE_TIMEOUT, 0, 0);
        }
        *outResult = 0;
        return true;
    }
    return false;
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
