//
// Created by brota on 03.09.2025.
//

#ifndef IDLEMONITOR_H
#define IDLEMONITOR_H

#define WM_IDLE_TIMEOUT (WM_USER + 100)

#include <windows.h>
#include <IComponent.h>

#include "win32/HashMapMessageHandler.h"

class IdleMonitor : public ui::IComponent {
public:
    void InitializeMessageHandlers();

    IdleMonitor();
    ~IdleMonitor() override;

    void OnCreate(HWND hwndParent) noexcept override;
    void OnDestroy() noexcept override;
    bool OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* outResult) noexcept override;

    void Pause() noexcept;
    void Resume() noexcept;
    [[nodiscard]] bool IsPaused() const noexcept { return isPaused_; }

    static ULONGLONG GetIdleTimeMs() noexcept;

private:
    HWND hwndParent_ = nullptr;
    bool isPaused_ = false;
    win32::HashMapMessageHandler messageHandler_;
    static constexpr UINT_PTR IDLE_TIMER_ID = 1001;
    static constexpr ULONGLONG IDLE_THRESHOLD_MS = 5000;
};
#endif //IDLEMONITOR_H
