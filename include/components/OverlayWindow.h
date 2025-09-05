#ifndef OVERLAYWINDOW_H
#define OVERLAYWINDOW_H

#include <IComponent.h>

#include <windows.h>
#include <string>
#include <iostream>
#include "win32/HashMapMessageHandler.h"

#define WM_OVERLAY_WINDOW_DESTROY (WM_USER + 101)

class OverlayWindow : public ui::IComponent {
public:
    OverlayWindow(HINSTANCE hInstance, std::wstring className);
    ~OverlayWindow() override;

    void OnCreate(HWND hwndParent) noexcept override;
    void OnDestroy() noexcept override;
    bool OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* outResult) noexcept override;

    bool RegisterClass();
    bool Create();
    void Destroy();

    HWND GetHandle() const { return hwnd_; }
    bool IsVisible() const { return isVisible_; }


    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


private:
    HINSTANCE hInstance_;
    std::wstring className_;
    HWND hwnd_ = nullptr;
    HWND parentHwnd_ = nullptr;
    bool isVisible_ = false;
    bool ignoreFirstInput_ = true;
    bool classRegistered_ = false;

    win32::HashMapMessageHandler messageHandler_;

    struct SpriteState {
        int x, y, dx, dy, w, h;
    };
    SpriteState* spriteState_ = nullptr;

    static constexpr UINT_PTR OVERLAY_ANIM_TIMER = 2001;

    void InitializeMessageHandlers();
};

#endif // OVERLAYWINDOW_H