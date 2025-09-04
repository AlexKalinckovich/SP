// Win32Window.h
#pragma once

#include <windows.h>
#include <string>
#include <memory>

#include "HashMapMessageHandler.h"
#include "components/MenuBar.h"
#include "components/OverlayWindow.h"
#include "utils/ComponentManager.h"
#include "utils/IdleMonitor.h"

namespace win32 {

    class Win32Window
    {
        public:
            Win32Window(HINSTANCE hInstance, std::wstring className, std::wstring windowTitle) noexcept;
            ~Win32Window() noexcept;

            bool Register() noexcept;
            bool Create(int x, int y, int width, int height, DWORD exStyle, DWORD style) noexcept;
            void Show(int nCmdShow = SW_SHOW) const noexcept;
            void Update() const noexcept;
            int ShowAndRun(int nCmdShow = SW_SHOW) const noexcept;

            void AddComponent(std::shared_ptr<ui::IComponent> c) noexcept;
            bool RemoveComponent(const ui::IComponent* cptr) noexcept;

            HWND GetHwnd() const { return hwnd_; }
            HINSTANCE GetHInstance() const { return hInstance_; }

        private:
            HINSTANCE hInstance_;
            std::wstring className_;
            std::wstring windowTitle_;
            HWND hwnd_ = nullptr;

            MenuBar menuBar_;
            std::shared_ptr<OverlayWindow> overlayWindow_;
            HashMapMessageHandler messageHandler_;
            ComponentManager componentManager_;
            std::shared_ptr<IdleMonitor> idleMonitor_;

            static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
            LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

            void InitializeMessageHandlers();
            void ShowAboutDialog() const;
            void CreateOverlayWindow();
    };

} // namespace win32