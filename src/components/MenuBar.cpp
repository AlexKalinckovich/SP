// MenuBar.cpp
#include "components/MenuBar.h"

#include <windows.h>

MenuBar::MenuBar() = default;

MenuBar::~MenuBar()
{
    Destroy();
}

bool MenuBar::Create()
{
    if (hMenuBar_ != nullptr)
    {
        Destroy();
    }

    hMenuBar_ = ::CreateMenu();
    if (hMenuBar_ == nullptr)
    {
        return false;
    }

    if (!CreateFileMenu(hMenuBar_)) return false;
    if (!CreateEditMenu(hMenuBar_)) return false;
    if (!CreateHelpMenu(hMenuBar_)) return false;

    return true;
}

void MenuBar::Destroy()
{
    if (hMenuBar_ != nullptr)
    {
        ::DestroyMenu(hMenuBar_);
        hMenuBar_ = nullptr;
    }
}

bool MenuBar::CreateFileMenu(HMENU hMenu)
{
    HMENU hFile = ::CreatePopupMenu();
    if (hFile == nullptr)
    {
        return false;
    }

    ::AppendMenuW(hFile, MF_STRING, MenuBar::ID_FILE_OPEN, L"Open...\tCtrl+O");
    ::AppendMenuW(hFile, MF_STRING, MenuBar::ID_FILE_SAVE, L"Save...\tCtrl+S");
    ::AppendMenuW(hFile, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(hFile, MF_STRING, MenuBar::ID_FILE_EXIT, L"Exit");

    ::AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hFile), L"File");
    return true;
}

bool MenuBar::CreateEditMenu(HMENU hMenu)
{
    HMENU hEdit = ::CreatePopupMenu();
    if (hEdit == nullptr)
    {
        return false;
    }

    ::AppendMenuW(hEdit, MF_STRING, MenuBar::ID_EDIT_CUT, L"Cut\tCtrl+X");
    ::AppendMenuW(hEdit, MF_STRING, MenuBar::ID_EDIT_COPY, L"Copy\tCtrl+C");
    ::AppendMenuW(hEdit, MF_STRING, MenuBar::ID_EDIT_PASTE, L"Paste\tCtrl+V");

    ::AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hEdit), L"Edit");
    return true;
}

bool MenuBar::CreateHelpMenu(HMENU hMenu)
{
    HMENU hHelp = ::CreatePopupMenu();
    if (hHelp == nullptr) return false;

    ::AppendMenuW(hHelp, MF_STRING, ID_HELP_ABOUT, L"About");
    ::AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hHelp), L"Help");
    return true;
}
