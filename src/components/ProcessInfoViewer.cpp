#include "components/ProcessInfoViewer.h"

#include <stdexcept>
#include <sstream>
#include <iomanip>

#include "meta_info/message_codes.h"
#include "meta_info/error_codes.h"
#include "utils/ErrorFormatter.h"

ProcessInfoViewer::ProcessInfoViewer(HINSTANCE hInstance, HWND hParent)
    : m_hWnd(nullptr),
      m_hParent(hParent),
      m_hInstance(hInstance),
      m_hProcessList(nullptr),
      m_hFindStringEdit(nullptr),
      m_hReplaceStringEdit(nullptr),
      m_hReplaceButton(nullptr),
      m_hCollectReadableButton(nullptr),
      m_hShowWritableButton(nullptr),
      m_hMemPatchDll(nullptr),
      m_pReplaceStringInMemory(nullptr),
      m_pReadMemoryBytes(nullptr),
      m_pFindPatternInMemory(nullptr),
      m_ResultData(std::nullopt)
{
    m_hMemPatchDll = LoadLibrary(L"C:\\Users\\brota\\CLionProjects\\MemPatch\\cmake-build-debug\\libMemPatch.dll");

    if (m_hMemPatchDll != nullptr)
    {
        m_pReplaceStringInMemory = (LPFN_REPLACESTRINGINMEMORY) GetProcAddress(m_hMemPatchDll, "ReplaceStringInMemory");
        m_pReadMemoryBytes       = (LPFN_READMEMORYBYTES) GetProcAddress(m_hMemPatchDll, "ReadMemoryBytes");
        m_pFindPatternInMemory   = (LPFN_FINDPATTERNINGMEMORY) GetProcAddress(m_hMemPatchDll, "FindPatternInMemory");
    }
}

constexpr wchar_t MOCK_TEST_STRING[] = L"PROCESS_VIEWER_TEST_STRING_ABCDEF";
constexpr wchar_t MOCK_TEST_REPLACE[] = L"PROCESS_VIEWER_REPLACED_ABCDEF";

ProcessInfoViewer::~ProcessInfoViewer()
{
    if (m_hMemPatchDll != nullptr)
    {
        FreeLibrary(m_hMemPatchDll);
    }
}

void ProcessInfoViewer::Register(HINSTANCE hInstance)
{
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = ProcessInfoViewer::WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = PROCESS_VIEWER_CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClass(&wc);
}

bool ProcessInfoViewer::ShowModal(const int x,
                                  const int y,
                                  const int width,
                                  const int height)
{
    m_hWnd = CreateWindowEx(
        WS_EX_TOPMOST,
        PROCESS_VIEWER_CLASS_NAME,
        L"Process Info",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, width, height,
        m_hParent,
        nullptr,
        m_hInstance,
        this
    );

    if (m_hWnd == nullptr)
    {
        return false;
    }

    EnableWindow(m_hParent, FALSE);
    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);

    MSG msg = {nullptr};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return true;
}

LRESULT CALLBACK ProcessInfoViewer::WindowProc(HWND hWnd, const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
{
    ProcessInfoViewer *pThis = nullptr;
    LRESULT lRes = 0;

    if (uMsg == WM_NCCREATE)
    {
        const CREATESTRUCT *pCreate = (CREATESTRUCT *) lParam;
        pThis = static_cast<ProcessInfoViewer*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) pThis);
        pThis->m_hWnd = hWnd;
        lRes = 1;
    }
    else
    {
        pThis = (ProcessInfoViewer *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }

    if (pThis != nullptr)
    {
        lRes = pThis->HandleMessage(uMsg, wParam, lParam);
    }
    else
    {
        lRes = DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return lRes;
}

LRESULT ProcessInfoViewer::HandleMessage(const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
{
    LRESULT lRes = 0;

    switch (uMsg)
    {
        case WM_CREATE:
            lRes = OnCreate(m_hWnd);
            break;
        case WM_COMMAND:
            lRes = OnCommand(wParam);
            break;
        case WM_CLOSE:
            lRes = OnClose();
            break;
        case WM_DESTROY:
            lRes = OnDestroy();
            break;
        default:
            lRes = DefWindowProc(m_hWnd, uMsg, wParam, lParam);
    }

    return lRes;
}

LRESULT ProcessInfoViewer::OnCreate(HWND hWnd)
{
    m_hProcessList = CreateWindowEx(
        WS_EX_CLIENTEDGE, L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_STANDARD,
        10, 10, 260, 200,
        hWnd, (HMENU)IDC_PROCESS_LIST, m_hInstance, nullptr);

    CreateWindowEx(
        0, L"STATIC", L"String to Find:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 220, 150, 20,
        hWnd, nullptr, m_hInstance, nullptr);

    m_hFindStringEdit = CreateWindowEx(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
        10, 240, 260, 25,
        hWnd, (HMENU)IDC_PATTERN_EDIT, m_hInstance, nullptr);
    
    CreateWindowEx(
        0, L"STATIC", L"String to Replace:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 275, 150, 20,
        hWnd, nullptr, m_hInstance, nullptr);
        
    m_hReplaceStringEdit = CreateWindowEx(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
        10, 295, 260, 25,
        hWnd, (HMENU)IDC_REPLACE_EDIT, m_hInstance, nullptr);

    m_hReplaceButton = CreateWindowEx(
        0, L"BUTTON", L"Replace String",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        10, 330, 125, 30,
        hWnd, (HMENU)IDC_REPLACE_BUTTON, m_hInstance, nullptr);

    m_hCollectReadableButton = CreateWindowEx(
        0, L"BUTTON", L"Collect Readable Memory",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        145, 330, 175, 30,
        hWnd, (HMENU)IDC_COLLECT_READABLE_BUTTON, m_hInstance, nullptr);

    m_hShowWritableButton = CreateWindowEx(
        0, L"BUTTON", L"Show Writable Memory",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, 370, 260, 30,
        hWnd, (HMENU)IDC_SHOW_WRITABLE_BUTTON, m_hInstance, nullptr);

    PopulateProcessList();
    
    return 0;
}

LRESULT ProcessInfoViewer::OnCommand(const WPARAM wParam) const
{
    const int wmId = LOWORD(wParam);
    int wmEvent = HIWORD(wParam);

    if (wmId == IDC_REPLACE_BUTTON)
    {
        OnReplaceStringClick();
    }
    else if (wmId == IDC_COLLECT_READABLE_BUTTON)
    {
        OnCollectReadableMemoryClick();
    }
    else if (wmId == IDC_SHOW_WRITABLE_BUTTON)
    {
        OnShowWritableMemoryClick();
    }

    return 0;
}

LRESULT ProcessInfoViewer::OnClose() const
{
    DestroyWindow(m_hWnd);
    return 0;
}

LRESULT ProcessInfoViewer::OnDestroy() const
{
    EnableWindow(m_hParent, TRUE);
    SetForegroundWindow(m_hParent);
    PostQuitMessage(0);
    return 0;


}

void ProcessInfoViewer::PopulateProcessList() const
{
    SendMessage(m_hProcessList, LB_RESETCONTENT, 0, 0);

    std::vector<PROCESSENTRY32> processes;
    const bool success = GetCurrentProcesses(processes);

    if (success)
    {
        const size_t count = processes.size();
        for (size_t i = 0; i < count; ++i)
        {
            PROCESSENTRY32 pe32 = processes[i];
            const LRESULT itemIndex = SendMessage(m_hProcessList, LB_ADDSTRING, 0, LPARAM(pe32.szExeFile));
            SendMessage(m_hProcessList, LB_SETITEMDATA, itemIndex, pe32.th32ProcessID);
        }
    }
}

void ProcessInfoViewer::OnReplaceStringClick() const
{
    if (m_pReplaceStringInMemory == nullptr)
    {
        MessageBox(m_hWnd, L"DLL function not loaded!", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    const int selIndex = static_cast<int>(SendMessage(m_hProcessList, LB_GETCURSEL, 0, 0));
    if (selIndex == LB_ERR)
    {
        MessageBox(m_hWnd, L"Please select a process.", L"Error", MB_OK | MB_ICONWARNING);
        return;
    }

    const DWORD processID = static_cast<DWORD>(SendMessage(m_hProcessList, LB_GETITEMDATA, selIndex, 0));

    const int findStringLen    = GetWindowTextLength(m_hFindStringEdit);
    const int replaceStringLen = GetWindowTextLength(m_hReplaceStringEdit);

    if (findStringLen == 0)
    {
        MessageBox(m_hWnd, L"Please enter a string to find.", L"Error", MB_OK | MB_ICONWARNING);
        return;
    }

    std::wstring findString(findStringLen + 1, L'\0');
    GetWindowText(m_hFindStringEdit, &findString[0], findStringLen + 1);

    std::wstring replaceString(replaceStringLen + 1, L'\0');
    GetWindowText(m_hReplaceStringEdit, &replaceString[0], replaceStringLen + 1);

    findString.resize(findStringLen);
    replaceString.resize(replaceStringLen);

    if (replaceString.length() > findString.length())
    {
        MessageBox(m_hWnd,
            L"Replacement string cannot be longer than the original string.",
            L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    HANDLE hProcess = OpenProcess(
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
        FALSE,
        processID
    );

    if (hProcess == nullptr)
    {
        const DWORD errorCode = GetLastError();
        std::wstring errorMsg = L"Failed to open process. Error: " + std::to_wstring(errorCode);
        if (errorCode == ERROR_ACCESS_DENIED)
        {
            errorMsg += L" (Access Denied - try running as Administrator)";
        }
        MessageBox(m_hWnd, errorMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    std::vector<MemoryRegion> writableRegions;
    const bool regionsSuccess = GetWritableMemoryRegions(processID, writableRegions);

    if (!regionsSuccess || writableRegions.empty())
    {
        CloseHandle(hProcess);
        MessageBox(m_hWnd, L"No writable memory regions found.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    bool replacementPerformed = false;
    SIZE_T totalReplacements = 0;

    const size_t writableRegionsSize = writableRegions.size();
    for (size_t i = 0; i < writableRegionsSize; ++i)
    {
        const MemoryRegion& region = writableRegions[i];

        std::vector<LPCVOID> foundAddresses;
        const SIZE_T foundCount = m_pFindPatternInMemory(
            hProcess,
            region.baseAddress,
            region.regionSize,
            (const BYTE*)(findString.c_str()),
            findString.length() * sizeof(wchar_t),
            foundAddresses
        );

        if (foundCount > 0)
        {
            std::wstring foundMsg = L"Found " + std::to_wstring(foundCount) +
                                   L" occurrences in region " + std::to_wstring(i + 1) +
                                   L" of " + std::to_wstring(writableRegions.size()) +
                                   L". Attempting replacement...";
            MessageBox(m_hWnd, foundMsg.c_str(), L"Found String", MB_OK | MB_ICONINFORMATION);

            const bool replaceSuccess = m_pReplaceStringInMemory(
                hProcess,
                region.baseAddress,
                region.regionSize,
                findString.c_str(),
                replaceString.c_str()
            );

            if (replaceSuccess)
            {
                replacementPerformed = true;
                totalReplacements += foundCount;
            }
        }
    }

    CloseHandle(hProcess);

    if (replacementPerformed)
    {
        const std::wstring successMsg = L"Successfully replaced " + std::to_wstring(totalReplacements) +
                                        L" occurrences of the string.";
        PostMessage(m_hParent, WM_STRING_REPLACE_RESULT, 0, 0);
        MessageBox(m_hWnd, successMsg.c_str(), L"Success", MB_OK | MB_ICONINFORMATION);
    }
    else
    {
        MessageBox(m_hWnd,
            L"Failed to replace string. The string may not exist in writable memory, or access was denied.",
            L"Error", MB_OK | MB_ICONERROR);
    }
}

void ProcessInfoViewer::OnCollectReadableMemoryClick() const
{
    const int selIndex = static_cast<int>(SendMessage(m_hProcessList, LB_GETCURSEL, 0, 0));
    if (selIndex == LB_ERR)
    {
        ErrorFormatter::SetLastApplicationError(U_ERROR_INVALID_PROCESS_SELECTION);
        MessageBox(m_hWnd, L"Please select a process.", L"Error", MB_OK | MB_ICONWARNING);
        return;
    }

    const DWORD processID = static_cast<DWORD>(SendMessage(m_hProcessList, LB_GETITEMDATA, selIndex, 0));

    std::vector<MemoryRegion> readableRegions;
    const bool regionsSuccess = GetReadableMemoryRegions(processID, readableRegions);

    if (!regionsSuccess || readableRegions.empty())
    {
        ErrorFormatter::SetLastApplicationError(U_ERROR_MEMORY_QUERY_FAILED);
        MessageBox(m_hWnd, L"No readable memory regions found.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processID);
    if (hProcess == nullptr)
    {
        ErrorFormatter::SetLastApplicationError(U_ERROR_PROCESS_ACCESS_DENIED);
        MessageBox(m_hWnd, L"Failed to open process for reading.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    std::vector<BYTE> collectedData;
    const size_t regionCount = readableRegions.size();
    for (size_t i = 0; i < regionCount; ++i)
    {
        const MemoryRegion& region = readableRegions[i];

        if (m_pReadMemoryBytes != nullptr)
        {
            std::vector<BYTE> regionData;
            const bool readSuccess = m_pReadMemoryBytes(hProcess, region.baseAddress, region.regionSize, regionData);
            if (readSuccess)
            {
                collectedData.insert(collectedData.end(), regionData.begin(), regionData.end());
            }
        }
    }

    CloseHandle(hProcess);

    if (!collectedData.empty())
    {
        std::vector<BYTE> *heapData = new std::vector(std::move(collectedData));
        PostMessage(m_hParent, WM_READABLE_MEMORY_DATA, 0, (LPARAM)(heapData));
    }
    else
    {
        ErrorFormatter::SetLastApplicationError(U_ERROR_MEMORY_READ_FAILED);
        MessageBox(m_hWnd, L"Failed to collect readable memory data.", L"Error", MB_OK | MB_ICONERROR);
    }
}

void ProcessInfoViewer::OnShowWritableMemoryClick() const
{
    const int selIndex = static_cast<int>(SendMessage(m_hProcessList, LB_GETCURSEL, 0, 0));
    if (selIndex == LB_ERR)
    {
        ErrorFormatter::SetLastApplicationError(U_ERROR_INVALID_PROCESS_SELECTION);
        MessageBox(m_hWnd, L"Please select a process.", L"Error", MB_OK | MB_ICONWARNING);
        return;
    }

    const DWORD processID = static_cast<DWORD>(SendMessage(m_hProcessList, LB_GETITEMDATA, selIndex, 0));

    std::vector<MemoryRegion> writableRegions;
    const bool regionsSuccess = GetWritableMemoryRegions(processID, writableRegions);

    if (!regionsSuccess || writableRegions.empty())
    {
        ErrorFormatter::SetLastApplicationError(U_ERROR_MEMORY_QUERY_FAILED);
        MessageBox(m_hWnd, L"No writable memory regions found.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processID);
    if (hProcess == nullptr)
    {
        ErrorFormatter::SetLastApplicationError(U_ERROR_PROCESS_ACCESS_DENIED);
        MessageBox(m_hWnd, L"Failed to open process for reading.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    std::vector<BYTE> writableData;
    const size_t regionCount = writableRegions.size();
    for (size_t i = 0; i < regionCount; ++i)
    {
        const MemoryRegion& region = writableRegions[i];

        if (m_pReadMemoryBytes != nullptr)
        {
            std::vector<BYTE> regionData;
            const bool readSuccess = m_pReadMemoryBytes(hProcess, region.baseAddress, region.regionSize, regionData);
            if (readSuccess)
            {
                writableData.insert(writableData.end(), regionData.begin(), regionData.end());
            }
        }
    }

    CloseHandle(hProcess);

    if (!writableData.empty())
    {
        std::vector<BYTE>* heapData = new std::vector<BYTE>(std::move(writableData));
        PostMessage(m_hParent, WM_WRITABLE_MEMORY_DATA, reinterpret_cast<WPARAM>(heapData), 0);
    }
    else
    {
        ErrorFormatter::SetLastApplicationError(U_ERROR_MEMORY_READ_FAILED);
        MessageBox(m_hWnd, L"Failed to read writable memory data.", L"Error", MB_OK | MB_ICONERROR);
    }
}
