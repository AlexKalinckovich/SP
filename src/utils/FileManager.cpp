
#include "utils/FileManager.h"
#include <commdlg.h>
#include <string>
#include <vector>
#include <memory>

#include "utils/EncodingDetector.h"

#define NO_CONTENT ""
#define NO_FILE_PATH L""
#define NO_ERROR_CODE ""
#define WRITE_CHUNK_SIZE 4096
FileManager::FileLoadResult FileManager::LoadFile()
{
    const std::optional<std::wstring> filepath = OpenFileDialog();
    FileLoadResult result;
    if(!filepath.has_value())
    {
        result.content = NO_CONTENT;
        result.encoding = Encoding::UNKNOWN,
        result.errorMessage = "File not selected";
        result.isSuccess = false;
    }
    else
    {
        result = LoadFile(filepath.value());
    }
    return result;
}

FileManager::FileLoadResult FileManager::LoadFile(const std::wstring& filePath)
{
    std::string outErrorMessage;
    const std::vector<UCHAR> buffer = ReadFileContent(filePath, outErrorMessage);
    FileLoadResult result;
    if(!outErrorMessage.empty())
    {
        result.filePath = NO_FILE_PATH;
        result.content = NO_CONTENT;
        result.encoding = Encoding::UNKNOWN,
        result.errorMessage = outErrorMessage;
        result.isSuccess = false;
    }
    else
    {
        result = ProcessFileContent(buffer, outErrorMessage);
        result.filePath = filePath;
    }
    return result;
}

std::optional<std::wstring> FileManager::OpenFileDialog()
{
    wchar_t filePath[MAX_PATH] = {};
    std::optional<std::wstring> result = std::nullopt;

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"All Files\0*.*\0Text Files\0*.txt\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&ofn) == TRUE)
    {
        result = std::wstring(filePath);
    }


    return result;
}

std::vector<UCHAR> FileManager::ReadFileContent(const std::wstring& filePath, std::string& outErrorMessage)
{
    std::vector<UCHAR> buffer;

    HANDLE hFile = CreateFileW(
        filePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        outErrorMessage = "Failed to open file: " + GetLastErrorString();
        return buffer;
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize))
    {
        outErrorMessage = "Failed to get file size: " + GetLastErrorString();
        CloseHandle(hFile);
        return buffer;
    }

    if (fileSize.QuadPart > 0)
    {
        const auto bufferSize = static_cast<size_t>(fileSize.QuadPart);
        buffer.reserve(bufferSize);

        std::vector<UCHAR> chunk(READ_CHUNK_SIZE);
        DWORD bytesRead = 0;

        while (ReadFile(hFile, chunk.data(), READ_CHUNK_SIZE, &bytesRead, nullptr) && bytesRead > 0)
        {
            buffer.insert(buffer.end(), chunk.begin(), chunk.begin() + bytesRead);
        }

        if (GetLastError() != ERROR_SUCCESS && GetLastError() != ERROR_HANDLE_EOF)
        {
            outErrorMessage = "Failed to read file: " + GetLastErrorString();
            buffer.clear();
        }
    }

    CloseHandle(hFile);
    return buffer;
}

FileManager::FileLoadResult FileManager::ProcessFileContent(const std::vector<UCHAR>& buffer, const std::string& errorMessage)
{
    FileLoadResult result;
    if (!errorMessage.empty())
    {
        result.content = NO_CONTENT;
        result.encoding = Encoding::UNKNOWN,
        result.errorMessage = errorMessage;
        result.isSuccess = false;
    }
    else if (buffer.empty())
    {
        result.content = NO_CONTENT;
        result.encoding = Encoding::UNKNOWN,
        result.errorMessage = "File is empty";
        result.isSuccess = false;
    }
    else
    {
        const Encoding encoding = EncodingDetector::Detect(buffer.data(), buffer.size());
        const std::string content = EncodingDetector::ConvertToUTF8(buffer, encoding);
        result.content = content;
        result.encoding = encoding;
        result.isSuccess = true;
        result.errorMessage = NO_ERROR_CODE;
    }
    return result;
}

std::string FileManager::GetLastErrorString()
{
    const DWORD errorCode = GetLastError();
    if (errorCode == 0)
    {
        return "Unknown error";
    }

    LPSTR messageBuffer = nullptr;
    const size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&messageBuffer), 0, nullptr);

    std::string result(messageBuffer, size);
    LocalFree(messageBuffer);

    return result;
}

FileManager::FileSaveResult FileManager::SaveFile(const std::string& content, const SaveEncoding encoding)
{
    const std::optional<std::wstring> filePath = SaveFileDialog();
    if (!filePath.has_value())
    {
        return {"No file selected", false};
    }
    return SaveFile(content, filePath.value(), encoding);
}

FileManager::FileSaveResult FileManager::SaveFile(const std::string& content, const std::wstring& filePath, const SaveEncoding encoding)
{
    if (filePath.empty())
    {
        return {"Invalid file path", false};
    }

    const std::vector<UCHAR> buffer = EncodingDetector::ConvertFromUTF8(content, encoding);

    if (buffer.empty() && !content.empty())
    {
        return {"Failed to convert content to target encoding", false};
    }


    if (std::string errorMessage; !WriteFileContent(filePath, buffer, errorMessage))
    {
        return {errorMessage, false};
    }

    return {"", true};
}

std::optional<std::wstring> FileManager::SaveFileDialog()
{
    wchar_t filePath[MAX_PATH] = {};

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"All Files\0*.*\0Text Files\0*.txt\0UTF-8 Files\0*.txt\0UTF-16 Files\0*.txt\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;

    std::optional<std::wstring> result = std::nullopt;
    if (GetSaveFileNameW(&ofn) == TRUE)
    {
        result = std::wstring(filePath);
    }

    return result;
}

bool FileManager::WriteFileContent(const std::wstring& filePath, const std::vector<UCHAR>& buffer, std::string& errorMessage)
{
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_WRITE, 0,
                               nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        errorMessage = "Failed to create file: " + GetLastErrorString();
        return false;
    }

    bool success = true;
    DWORD totalBytesWritten = 0;
    constexpr DWORD chunkSize = WRITE_CHUNK_SIZE;

    while (totalBytesWritten < buffer.size())
    {
        DWORD bytesToWrite = static_cast<DWORD>(std::min(static_cast<size_t>(chunkSize),
                                                         buffer.size() - totalBytesWritten));
        DWORD bytesWritten = 0;

        if (!WriteFile(hFile, buffer.data() + totalBytesWritten, bytesToWrite, &bytesWritten, nullptr))
        {
            errorMessage = "Failed to write file: " + GetLastErrorString();
            success = false;
            break;
        }

        if (bytesWritten != bytesToWrite)
        {
            errorMessage = "Incomplete write operation";
            success = false;
            break;
        }

        totalBytesWritten += bytesWritten;
    }

    if (!FlushFileBuffers(hFile))
    {
        errorMessage = "Failed to flush file buffers: " + GetLastErrorString();
        success = false;
    }

    CloseHandle(hFile);

    if (!success)
    {
        DeleteFileW(filePath.c_str());
    }

    return success;
}

std::string FileManager::ConvertWStringToStdString(const std::wstring& content)
{
    const wchar_t *wContent = content.c_str();

    const size_t len = wcstombs(nullptr, wContent, 0) + 1;
    char* buffer = new char[len];

    wcstombs(buffer, wContent, len);
    std::string strContent(buffer);

    delete[] buffer;

    return strContent;
}
