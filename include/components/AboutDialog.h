#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <IComponent.h>
#include <windows.h>

#include <string>
#include "win32/HashMapMessageHandler.h"

class AboutDialog : public ui::IComponent
{
    public:
        AboutDialog(HINSTANCE hInstance, HWND parent);
        ~AboutDialog() override;

        void Show();

        void OnCreate(HWND hwndParent) noexcept override;
        void OnDestroy() noexcept override;
        bool OnMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* outResult) noexcept override;

    private:
        HINSTANCE hInstance_;
        HWND parent_;
        HWND dlgWindow_ = nullptr;

        static constexpr int ID_ABOUT_OK = 1;

        win32::HashMapMessageHandler messageHandler_;

        void CreateDialogWindow();
        void RunMessageLoop() const;
        static LRESULT CALLBACK StaticDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        LRESULT DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        LRESULT CreateHandler(WPARAM, LPARAM) const;
        void InitializeMessageHandlers();
};

#endif //ABOUTDIALOG_H
