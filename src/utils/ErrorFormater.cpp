#include "utils/ErrorFormater.h"
#include "meta_info/error_codes.h"
#include <string>
#include <sstream>
#include <unordered_map>

static const std::unordered_map<DWORD, std::string_view> g_appErrorDescriptions = {
    {U_ERROR_FILE_NOT_FOUND, "File not found"},
    {U_ERROR_USER_CANCELLED, "User cancelled the operation"},
    {U_ERROR_FILE_EMPTY, "File is empty"},
    {U_ERROR_FILE_TOO_LARGE, "File is too large for this operation"},
    {U_ERROR_FILE_WRITE_INCOMPLETE, "Failed to write all data to file (disk full?)"},
    {U_ERROR_ENCODING_CONVERSION_FAILED, "Failed to convert text encoding"},
    {U_ERROR_INVALID_FILE_PATH, "An invalid file path was provided"}
};

static const std::unordered_map<DWORD, std::wstring_view> g_appErrorDescriptionsW = {
    {U_ERROR_FILE_NOT_FOUND, L"File not found"},
    {U_ERROR_USER_CANCELLED, L"User cancelled the operation"},
    {U_ERROR_FILE_EMPTY, L"File is empty"},
    {U_ERROR_FILE_TOO_LARGE, L"File is too large for this operation"},
    {U_ERROR_FILE_WRITE_INCOMPLETE, L"Failed to write all data to file (disk full?)"},
    {U_ERROR_ENCODING_CONVERSION_FAILED, L"Failed to convert text encoding"},
    {U_ERROR_INVALID_FILE_PATH, L"An invalid file path was provided"}
};

std::string ErrorFormatter::GetLastErrorString()
{
    const DWORD errorCode = ::GetLastError();
    return GetErrorString(errorCode);
}

std::string ErrorFormatter::GetErrorString(const DWORD errorCode)
{
    std::string result;
    if (errorCode == 0)
    {
        result = "Success";
    }
    else if (IS_APPLICATION_ERROR(errorCode))
    {
        result = FormatApplicationError(errorCode);
    }
    else
    {
        result = FormatSystemError(errorCode);
    }
    return result;
}

std::string ErrorFormatter::FormatSystemError(DWORD errorCode)
{
    // ... (existing code remains the same) ...
    LPSTR messageBuffer = nullptr;
    const DWORD size = ::FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&messageBuffer),
        0,
        nullptr
    );
    std::string result = "Unknown system error: " + std::to_string(errorCode);

    if (size != 0)
    {
        std::string error_string(messageBuffer, size);

        bool isEndSymbol = false;
        while (!error_string.empty() && !isEndSymbol)
        {
            error_string.pop_back();
            if (!error_string.empty())
            {
                const char lastChar = error_string.back();
                isEndSymbol = (lastChar == '\n' || lastChar == '\r' || lastChar == '.' || lastChar == ' ');
            }
        }

        ::LocalFree(messageBuffer);
        result = error_string;
    }
    return result;
}

std::string ErrorFormatter::FormatApplicationError(const DWORD errorCode)
{
    const DWORD baseError = errorCode & ~ERROR_APP_BASE;
    const std::string description = GetApplicationErrorDescription(errorCode);
    
    std::ostringstream oss;
    oss << "Application Error [" << std::hex << errorCode << "]: " << "Base Error [" << std::hex << baseError << "]: "<< description;
    return oss.str();
}

std::string ErrorFormatter::GetApplicationErrorDescription(DWORD appErrorCode)
{
    const std::unordered_map<DWORD, std::string_view>::const_iterator it =
        g_appErrorDescriptions.find(appErrorCode);
    if (it != g_appErrorDescriptions.end())
    {
        return std::string(it->second);
    }
    return "Unknown application error";
}

std::wstring ErrorFormatter::GetLastErrorStringW()
{
    const DWORD errorCode = ::GetLastError();
    return GetErrorStringW(errorCode);
}

std::wstring ErrorFormatter::GetErrorStringW(DWORD errorCode)
{
    if (errorCode == 0)
    {
        return L"Success";
    }

    if (IS_APPLICATION_ERROR(errorCode))
    {
        return FormatApplicationErrorW(errorCode);
    }
    
    return FormatSystemErrorW(errorCode);
}

std::wstring ErrorFormatter::FormatSystemErrorW(DWORD errorCode)
{
    LPWSTR messageBuffer = nullptr;
    const DWORD size = ::FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, 
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&messageBuffer),
        0, 
        nullptr
    );

    if (size == 0)
    {
        return L"Unknown system error: " + std::to_wstring(errorCode);
    }

    std::wstring result(messageBuffer, size);
    
    while (!result.empty() && (result.back() == L'\n' || result.back() == L'\r' || result.back() == L'.' || result.back() == L' '))
    {
        result.pop_back();
    }
    
    ::LocalFree(messageBuffer);
    return result;
}

std::wstring ErrorFormatter::FormatApplicationErrorW(const DWORD errorCode)
{
    const DWORD baseError = errorCode & ~ERROR_APP_BASE;
    const std::wstring description = GetApplicationErrorDescriptionW(errorCode);
    
    std::wostringstream woss;
    woss << "Application Error [" << std::hex << errorCode << "]: " << "Base Error [" << std::hex << baseError << "]: "<< description;
    return woss.str();
}

std::wstring ErrorFormatter::GetApplicationErrorDescriptionW(const DWORD appErrorCode)
{
    const std::unordered_map<DWORD, std::wstring_view>::const_iterator it =
        g_appErrorDescriptionsW.find(appErrorCode);
    if (it != g_appErrorDescriptionsW.end())
    {
        return std::wstring(it->second);
    }
    return L"Unknown application error";
}

bool ErrorFormatter::SetLastApplicationError(const DWORD appErrorCode)
{
    if (!IS_APPLICATION_ERROR(appErrorCode))
    {
        return false;
    }
    
    ::SetLastErrorEx(appErrorCode, 0);
    return true;
}

bool ErrorFormatter::IsApplicationError(DWORD errorCode)
{
    return IS_APPLICATION_ERROR(errorCode);
}

bool ErrorFormatter::IsSystemError(DWORD errorCode)
{
    return IS_SYSTEM_ERROR(errorCode);
}

void ErrorFormatter::ClearError()
{
    ::SetLastError(0);
}