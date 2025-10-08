// FontSelectorDialog.cpp
#include "components/FontSelectorComponent.h"


// Control IDs
#define IDC_FONT_LISTBOX_MODAL 2001
#define IDC_OK_BUTTON          2002
#define IDC_CANCEL_BUTTON      2003

FontSelectorDialog::FontSelectorDialog(HINSTANCE hInstance, HWND hwndParent, std::filesystem::path fontDirectory)
    : m_hInstance(hInstance),
      m_hParent(hwndParent),
      m_fontDirectory(std::move(fontDirectory)),
      m_hDialog(nullptr),
      m_hListBox(nullptr),
      m_hOkButton(nullptr),
      m_hCancelButton(nullptr)
{
    ScanFontDirectory();
}

void FontSelectorDialog::RegisterDialogClass(HINSTANCE hInstance)
{
    if (s_classRegistered) return;

    WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = DialogWndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = sizeof(LONG_PTR);
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName  = nullptr;
    wcex.lpszClassName = s_wndClassName;

    if (RegisterClassExW(&wcex))
    {
        s_classRegistered = true;
    }
}

std::optional<std::wstring> FontSelectorDialog::ShowModal()
{
    RegisterDialogClass(m_hInstance);

    // Create the dialog window. Note the WS_POPUPWINDOW style.
    m_hDialog = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        s_wndClassName,
        L"Select Font",
        WS_POPUPWINDOW | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 400,
        m_hParent,
        nullptr,
        m_hInstance,
        this // Pass 'this' pointer to be retrieved in WM_CREATE
    );

    if (!m_hDialog) return std::nullopt;

    CenterWindow();
    ShowWindow(m_hDialog, SW_SHOW);
    UpdateWindow(m_hDialog);

    // 1. Disable the parent window.
    // 2. Run a message loop until our dialog is closed.
    // 3. Re-enable the parent window.
    EnableWindow(m_hParent, FALSE);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!IsDialogMessage(m_hDialog, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    EnableWindow(m_hParent, TRUE);
    SetForegroundWindow(m_hParent);

    return m_selectedFont;
}


LRESULT CALLBACK FontSelectorDialog::DialogWndProc(HWND hwnd, const UINT msg, const WPARAM wParam, const LPARAM lParam)
{
    FontSelectorDialog* pThis = nullptr;

    if (msg == WM_NCCREATE)
    {
        auto pCreate = (CREATESTRUCT*)lParam;
        pThis = (FontSelectorDialog*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    }
    else
    {
        pThis = (FontSelectorDialog*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis)
    {
        return pThis->HandleMessage(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}


LRESULT FontSelectorDialog::HandleMessage(HWND hwnd, const UINT msg, const WPARAM wParam, const LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            m_hDialog = hwnd; 

            m_hListBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
                                         WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | LBS_SORT,
                                         10, 10, 265, 300, hwnd, (HMENU) IDC_FONT_LISTBOX_MODAL, m_hInstance, nullptr);

            m_hOkButton = CreateWindowExW(0, L"BUTTON", L"OK",
                                          WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                                          115, 320, 80, 25, hwnd, (HMENU) IDC_OK_BUTTON, m_hInstance, nullptr);

            m_hCancelButton = CreateWindowExW(0, L"BUTTON", L"Cancel",
                                              WS_CHILD | WS_VISIBLE,
                                              200, 320, 80, 25, hwnd, (HMENU) IDC_CANCEL_BUTTON, m_hInstance, nullptr);

            auto hGuiFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
            SendMessage(m_hListBox, WM_SETFONT, (WPARAM) hGuiFont, TRUE);
            SendMessage(m_hOkButton, WM_SETFONT, (WPARAM) hGuiFont, TRUE);
            SendMessage(m_hCancelButton, WM_SETFONT, (WPARAM) hGuiFont, TRUE);

            PopulateListBox();
            return 0;
        }

        case WM_COMMAND:
        {
            const WORD controlId = LOWORD(wParam);
            const WORD notifyCode = HIWORD(wParam);

            if (controlId == IDC_OK_BUTTON || (controlId == IDC_FONT_LISTBOX_MODAL && notifyCode == LBN_DBLCLK))
            {
                const LRESULT index = SendMessage(m_hListBox, LB_GETCURSEL, 0, 0);
                if (index != LB_ERR)
                {
                    LRESULT len = SendMessage(m_hListBox, LB_GETTEXTLEN, index, 0);
                    std::wstring text(len, L'\0');
                    SendMessage(m_hListBox, LB_GETTEXT, index, (LPARAM) text.data());
                    m_selectedFont = text;
                }
                DestroyWindow(hwnd);
            }
            else if (controlId == IDC_CANCEL_BUTTON)
            {
                m_selectedFont.reset(); 
                DestroyWindow(hwnd);
            }
            return 0;
        }

        case WM_CLOSE:
            m_selectedFont.reset();
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// --- Helper implementations ---

void FontSelectorDialog::ScanFontDirectory()
{
    m_fontNames.clear();
    try
    {
        if (!std::filesystem::exists(m_fontDirectory) || !std::filesystem::is_directory(m_fontDirectory)) return;

        for (const auto& entry : std::filesystem::directory_iterator(m_fontDirectory))
        {
            if (entry.is_regular_file() && entry.path().extension() == L".ttf")
            {
                m_fontNames.push_back(entry.path().stem().wstring());
            }
        }
    }
    catch (const std::filesystem::filesystem_error&) {}
}

void FontSelectorDialog::PopulateListBox() const
{
    if (!m_hListBox) return;
    for (const std::wstring &name: m_fontNames)
    {
        SendMessage(m_hListBox, LB_ADDSTRING, 0, (LPARAM)name.c_str());
    }
}

void FontSelectorDialog::CenterWindow() const
{
    RECT rcParent, rcSelf;
    GetWindowRect(m_hParent, &rcParent);
    GetWindowRect(m_hDialog, &rcSelf);

    const int x = rcParent.left + (rcParent.right - rcParent.left - (rcSelf.right - rcSelf.left)) / 2;
    const int y = rcParent.top + (rcParent.bottom - rcParent.top - (rcSelf.bottom - rcSelf.top)) / 2;

    SetWindowPos(m_hDialog, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);
}
