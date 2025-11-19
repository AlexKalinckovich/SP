#pragma once

#include <optional>
#include <windows.h>
#include <vector>
#include <string>
#include "C:/Users/brota/CLionProjects/ProcessLib/processInfo.h"

#define PROCESS_VIEWER_CLASS_NAME L"ProcessInfoViewerClass"

typedef bool(WINAPI* LPFN_REPLACESTRINGINMEMORY)(HANDLE, LPVOID, SIZE_T, LPCWSTR, LPCWSTR);
typedef bool(WINAPI* LPFN_READMEMORYBYTES)(HANDLE, LPCVOID, SIZE_T, std::vector<BYTE>&);
typedef SIZE_T(WINAPI* LPFN_FINDPATTERNINGMEMORY)(HANDLE, LPCVOID, SIZE_T, const BYTE*, SIZE_T, std::vector<LPCVOID>&);

class ProcessInfoViewer {
    public:
        ProcessInfoViewer(HINSTANCE hInstance, HWND hParent);
        ~ProcessInfoViewer();

        static void Register(HINSTANCE hInstance);
        bool ShowModal(int x, int y, int width, int height);

    private:
        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
        LRESULT OnCreate(HWND hWnd);
        LRESULT OnCommand(WPARAM wParam) const;
        [[nodiscard]] LRESULT OnDestroy() const;
        [[nodiscard]] LRESULT OnClose() const;

        void PopulateProcessList() const;
        void OnReplaceStringClick() const;
        void OnCollectReadableMemoryClick() const;
        void OnShowWritableMemoryClick() const;

        HWND m_hWnd;
        HWND m_hParent;
        HINSTANCE m_hInstance;
        HWND m_hProcessList;
        HWND m_hFindStringEdit;
        HWND m_hReplaceStringEdit;
        HWND m_hReplaceButton;
        HWND m_hCollectReadableButton;
        HWND m_hShowWritableButton;

        HINSTANCE m_hMemPatchDll;
        LPFN_REPLACESTRINGINMEMORY m_pReplaceStringInMemory;
        LPFN_READMEMORYBYTES m_pReadMemoryBytes;
        LPFN_FINDPATTERNINGMEMORY m_pFindPatternInMemory;

        std::optional<std::vector<BYTE>> m_ResultData;
};