#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

#include "win32/Win32Window.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nShowCmd)
{
    const std::wstring className = L"MySampleClass";
    const std::wstring windowTitle = L"Text redactor";

    win32::Win32Window mainWin(hInstance, className, windowTitle);

    if (!mainWin.Register())
    {
        MessageBoxW(nullptr, L"RegisterClassExW failed", L"Error", MB_ICONERROR);
        return -1;
    }

    constexpr DWORD style = WS_OVERLAPPEDWINDOW;
    constexpr DWORD exStyle = 0;
    if (!mainWin.Create(100, 100, 800, 600, exStyle, style))
    {
        MessageBoxW(nullptr, L"CreateWindowExW failed", L"Error", MB_ICONERROR);
        return -1;
    }

    return mainWin.ShowAndRun(nShowCmd);
}

extern "C" int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    return wWinMain(hInstance, hPrevInstance, ::GetCommandLineW(), nCmdShow);
}