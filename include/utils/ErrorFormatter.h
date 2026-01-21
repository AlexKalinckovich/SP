// ErrorFormatter.h
#pragma once

#include <windows.h>
#include <string>

class ErrorFormatter
{
public:
    static std::string GetLastErrorString();
    static std::string GetErrorString(DWORD errorCode);
    static std::wstring GetLastErrorStringW();
    static std::wstring GetErrorStringW(DWORD errorCode);

    static bool SetLastApplicationError(DWORD appErrorCode);
    static bool IsApplicationError(DWORD errorCode);
    static bool IsSystemError(DWORD errorCode);
    static void ClearError();

private:
    static std::string FormatSystemError(DWORD errorCode);
    static std::string FormatApplicationError(DWORD errorCode);
    static std::wstring FormatSystemErrorW(DWORD errorCode);
    static std::wstring FormatApplicationErrorW(DWORD errorCode);

    static std::string GetApplicationErrorDescription(DWORD appErrorCode);
    static std::wstring GetApplicationErrorDescriptionW(DWORD appErrorCode);
};