// FontSelectorDialog.h
#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <optional>

class FontSelectorDialog
{
public:
    FontSelectorDialog(HINSTANCE hInstance, HWND hwndParent, std::filesystem::path fontDirectory);
    std::optional<std::wstring> ShowModal();
    FontSelectorDialog(const FontSelectorDialog&) = delete;
    FontSelectorDialog& operator=(const FontSelectorDialog&) = delete;

private:
    static LRESULT CALLBACK DialogWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static void RegisterDialogClass(HINSTANCE hInstance);

    void ScanFontDirectory();
    void PopulateListBox() const;
    void CenterWindow() const;

    HINSTANCE m_hInstance;
    HWND m_hParent;
    HWND m_hDialog;
    HWND m_hListBox;
    HWND m_hOkButton;
    HWND m_hCancelButton;

    std::filesystem::path m_fontDirectory;
    std::vector<std::wstring> m_fontNames;
    std::optional<std::wstring> m_selectedFont;

    inline static const wchar_t* s_wndClassName = L"FontSelectorDialogClass";
    inline static bool s_classRegistered = false;
};