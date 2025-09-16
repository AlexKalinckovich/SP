// TextEditorComponent.cpp
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <fstream>
#include <iostream>
#include <commdlg.h>
#include <richedit.h>

#include "components/TextEditorComponent.h"

#include "utils/FileManager.h"

#define CALCULATE_NULL_TERMINATED_STRING (-1)


TextEditorComponent::TextEditorComponent(HINSTANCE hInstance)
    : hInstance_(hInstance)
{
    InitializeMessageHandlers();
}

TextEditorComponent::~TextEditorComponent()
{
    TextEditorComponent::OnDestroy();
}

void TextEditorComponent::OnCreate(HWND hwndParent) noexcept
{
    hwndParent_ = hwndParent;
    CreateEditControl();
}

void TextEditorComponent::InitializeMessageHandlers()
{
    messageHandler_.RegisterHandler(WM_SIZE, [this](HWND hwnd, WPARAM, const LPARAM lParam) -> LRESULT
    {
        bool res = false;
        if (hwnd == hwndParent_)
        {
            const int width = LOWORD(lParam);
            const int height = HIWORD(lParam);
            ResizeEditControl(width, height);
            res = true;
        }
        return res;
    });
    messageHandler_.RegisterHandler(WM_TIMER, [this](HWND hwnd, WPARAM wParam, LPARAM) -> LRESULT
    {
        if (wParam == IDT_TIMER_SAVE_PROMPT)
        {
            ::KillTimer(hwnd, IDT_TIMER_SAVE_PROMPT);
            UpdateModificationState();
            return TRUE;
        }
        return FALSE;
    });

    messageHandler_.RegisterHandler(WM_COMMAND, [this](HWND hwnd, const WPARAM wParam, const LPARAM lParam) -> LRESULT
    {
        if (lParam == reinterpret_cast<LPARAM>(hEditControl_))
        {
            switch (HIWORD(wParam))
            {
                case EN_CHANGE:
                    ::SetTimer(hwnd, IDT_TIMER_SAVE_PROMPT, SAVE_PROMPT_DELAY_MS, nullptr);
                    return TRUE;
                default:
                    return FALSE;
            }
        }
        return FALSE;
    });
}


void TextEditorComponent::OnDestroy() noexcept {
    DestroyEditControl();
}

bool TextEditorComponent::OnMessage(HWND hwnd,
                                    const UINT msg,
                                    const WPARAM wParam,
                                    const LPARAM lParam,
                                    LRESULT* outResult) noexcept {
    bool res = false;
    if (hEditControl_ && (hwnd == hEditControl_ || hwnd == hwndParent_)) {
        res = messageHandler_.HandleMessage(hwnd, msg, wParam, lParam);
    }
    return res;
}

void TextEditorComponent::CreateEditControl()
{
    HMODULE hRichEdit = ::LoadLibraryW(L"Msftedit.dll");
    const wchar_t* editClass = hRichEdit ? L"RICHEDIT50W" : L"EDIT";

    RECT clientRect;
    ::GetClientRect(hwndParent_, &clientRect);
    const int width  = clientRect.right - clientRect.left;
    const int height = clientRect.bottom - clientRect.top;

    constexpr DWORD wndStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_SAVESEL | ES_WANTRETURN;


    hEditControl_ = ::CreateWindowExW(
        WS_EX_CLIENTEDGE,
        editClass,
        L"",
        wndStyle,
        0, 0, width, height,
        hwndParent_,
        nullptr,
        hInstance_,
        nullptr
    );

    if (!hEditControl_)
    {
        const DWORD err = ::GetLastError();
        std::wcerr << L"Failed to create edit control: " << err << std::endl;
        return;
    }



    HFONT font = ::CreateFontW(
        DEFAULT_FONT_SIZE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, VARIABLE_PITCH, DEFAULT_FONT
    );
    ::PostMessageW(hEditControl_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);

    constexpr int leftMargin = 4;
    constexpr int rightMargin = 4;
    ::PostMessageW(hEditControl_, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(leftMargin, rightMargin));

    RECT rc = { leftMargin, 2, width - rightMargin, height - 2 };

    originalContent_ = GetText();
    hasUnsavedChanges_ = false;

    ::PostMessageW(hEditControl_, EM_SETRECT, 0, reinterpret_cast<LPARAM>(&rc));

    ::PostMessageW(hEditControl_, EM_SETMODIFY, FALSE, 0);

    ::PostMessageW(hEditControl_, EM_EMPTYUNDOBUFFER, 0, 0);

    ::SetWindowTextW(hEditControl_, L"Welcome to Text Editor!\r\n\r\nStart typing here...");

    ::ShowScrollBar(hEditControl_, SB_VERT, TRUE);
}


void TextEditorComponent::DestroyEditControl()
{
    if (hEditFont_)
    {
        ::DeleteObject(hEditFont_);
        hEditFont_ = nullptr;
    }

    if (hEditControl_)
    {
        ::DestroyWindow(hEditControl_);
        hEditControl_ = nullptr;
    }
}

void TextEditorComponent::ResizeEditControl(const int width, const int height) const
{
    if (!hEditControl_) return;
    ::SetWindowPos(hEditControl_, nullptr, 0, 0, width, height,
                   SWP_NOZORDER | SWP_NOMOVE | SWP_SHOWWINDOW);
    RECT rc = {4, 2, width - 4, height - 2};
    ::PostMessageW(hEditControl_, EM_SETRECT, 0, reinterpret_cast<LPARAM>(&rc));
}


bool TextEditorComponent::LoadFile() const
{
    const FileManager::FileLoadResult res = FileManager::LoadFile(hEditControl_);
    if (!hEditControl_ || !IsWindow(hEditControl_))
    {
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
        return false;
    }

    const std::string content = res.content;
    const int wideSize = MultiByteToWideChar(CP_UTF8, 0, content.c_str(), CALCULATE_NULL_TERMINATED_STRING, nullptr, 0);
    std::wstring wideContent(wideSize, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, content.c_str(), CALCULATE_NULL_TERMINATED_STRING, &wideContent[0], wideSize);

    ::SendMessageW(hEditControl_, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(wideContent.c_str()));

    const_cast<TextEditorComponent*>(this)->currentFilePath_ = res.filePath;
    const_cast<TextEditorComponent*>(this)->originalContent_ = wideContent;
    const_cast<TextEditorComponent*>(this)->hasUnsavedChanges_ = false;

    return true;
}

bool TextEditorComponent::SaveFile() const
{
    if (!hEditControl_)
    {
        return false;
    }

    const std::wstring content = GetText();
    const std::string utf8Content = FileManager::ConvertWStringToStdString(content);

    FileManager::FileSaveResult saveResult;
    if (currentFilePath_.empty())
    {
        saveResult = FileManager::SaveFile(hEditControl_, utf8Content, SaveEncoding::UTF8);
    }
    else
    {
        saveResult = FileManager::SaveFile(utf8Content, currentFilePath_, SaveEncoding::UTF8);
    }

    if (saveResult.isSuccess)
    {
        const_cast<TextEditorComponent *>(this)->MarkAsSaved();
    }

    return saveResult.isSuccess;
}

void TextEditorComponent::SetFont(const std::wstring& fontName, const int fontSize)
{
    if (hEditFont_)
    {
        ::DeleteObject(hEditFont_);
        hEditFont_ = nullptr;
    }

    hEditFont_ = ::CreateFontW(
        fontSize, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        fontName.c_str()
    );

    if (hEditFont_ && hEditControl_)
    {
        ::PostMessageW(hEditControl_, WM_SETFONT, reinterpret_cast<WPARAM>(hEditFont_), TRUE);
    }
}

void TextEditorComponent::Clear() const
{
    if (hEditControl_)
    {
        ::PostMessageW(hEditControl_, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L""));
    }
}

std::wstring TextEditorComponent::GetText() const
{
    if (!hEditControl_)
    {
        return L"";
    }

    const int textLength = ::GetWindowTextLengthW(hEditControl_);
    if (textLength <= 0)
    {
        return L"";
    }

    std::wstring content(textLength + 1, L'\0');
    ::GetWindowTextW(hEditControl_, &content[0], textLength + 1);
    content.resize(textLength);

    return content;
}

void TextEditorComponent::OnCut() const
{
    if (hEditControl_)
    {
        ::PostMessageW(hEditControl_, WM_CUT, 0, 0);
    }
}

void TextEditorComponent::OnCopy() const
{
    if (hEditControl_)
    {
        ::PostMessageW(hEditControl_, WM_COPY, 0, 0);
    }
}

void TextEditorComponent::OnPaste() const
{
    if (hEditControl_)
    {
        ::PostMessageW(hEditControl_, WM_PASTE, 0, 0);
    }
}

void TextEditorComponent::OnUndo() const
{
    if (hEditControl_)
    {
        ::PostMessageW(hEditControl_, WM_UNDO, 0, 0);
    }
}

void TextEditorComponent::OnRedo() const
{
    if (hEditControl_)
    {
        ::PostMessageW(hEditControl_, EM_REDO, 0, 0);
    }
}

void TextEditorComponent::OnSelectAll() const
{
    if (hEditControl_)
    {
        ::PostMessageW(hEditControl_, EM_SETSEL, 0, -1);
    }
}

bool TextEditorComponent::HasUnsavedChanges() const {
    return hasUnsavedChanges_;
}

void TextEditorComponent::MarkAsSaved() {
    hasUnsavedChanges_ = false;
    originalContent_ = GetText();
}

void TextEditorComponent::MarkAsModified() {
    hasUnsavedChanges_ = true;
}

void TextEditorComponent::UpdateModificationState() {
    if (!hEditControl_) return;

    const std::wstring currentText = GetText();
    hasUnsavedChanges_ = currentText != originalContent_;
}

bool TextEditorComponent::PromptSaveIfNeeded(HWND hwndParent) {
    if (!HasUnsavedChanges()) return true;

    const int result = ::MessageBoxW(hwndParent,
                                     L"You have unsaved changes. Do you want to save them before closing?",
                                     L"Unsaved Changes",
                                     MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON1);

    switch (result) {
        case IDYES:
            return SaveChanges();
        case IDNO:
            return true;
        case IDCANCEL:
        default:
            return false;
    }
}

bool TextEditorComponent::SaveChanges()
{
    if (currentFilePath_.empty())
    {
        return SaveFile();
    }

    const std::wstring content = GetText();
    const std::string utf8Content = FileManager::ConvertWStringToStdString(content);
    const FileManager::FileSaveResult result = FileManager::SaveFile(utf8Content, currentFilePath_, SaveEncoding::UTF8);

    if (result.isSuccess)
    {
        MarkAsSaved();
    }

    return result.isSuccess;
}

void TextEditorComponent::HandleTextChange()
{
    MarkAsModified();

    if (hwndParent_)
    {
        std::wstring title = L"Text Editor";
        title += L" *";
        ::SetWindowTextW(hwndParent_, title.c_str());
    }
}
