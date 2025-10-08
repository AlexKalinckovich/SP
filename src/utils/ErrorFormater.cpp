#include "utils/ErrorFormater.h"

std::string ErrorFormater::GetLastErrorString()
{
    const DWORD errorCode = GetLastError();
    if (errorCode == 0)
    {
        return "Unknown error";
    }

    return GetErrorString(errorCode);
}
std::string ErrorFormater::GetErrorString(const DWORD err)
{
    LPSTR messageBuffer = nullptr;
    const size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&messageBuffer), 0, nullptr);

    std::string result(messageBuffer, size);
    LocalFree(messageBuffer);

    return result;
}
