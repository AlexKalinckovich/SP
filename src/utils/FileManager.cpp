
#include "utils/FileManager.h"
#include <commdlg.h>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "utils/EncodingDetector.h"
#include "utils/ErrorFormater.h"

#define NO_CONTENT ""
#define NO_FILE_PATH L""
#define NO_ERROR_CODE ""
#define WRITE_CHUNK_SIZE 4096
#define MAX_FILE_PATH 1024
#define USER_CANCEL_DIALOG_ERROR 1
#define NO_SHARING_FILE_TO_OTHER_PROCESS_UNTIL_CLOSE 0

#define CALCULATE_NULL_TERMINATED_STRING (-1)
#define NO_OUTPUT_BUFFER nullptr
#define ZERO_MULTI_BYTE 0
#define NO_CHILD_DESCRIPTOR_INHERITANCE nullptr

FileManager::FileLoadResult FileManager::LoadFile(HWND hwnd)
{
    const std::optional<std::wstring> filepath = OpenFileDialog(hwnd);
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

std::optional<std::wstring> FileManager::OpenFileDialog(HWND hwnd)
{
    std::vector<WCHAR> buffer(MAX_FILE_PATH, L'\0');

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = buffer.data();
    ofn.nMaxFile = static_cast<DWORD>(buffer.size());
    ofn.lpstrFilter = L"All Files\0*.*\0Text Files\0*.txt\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    std::optional<std::wstring> result = std::nullopt;
    if (GetOpenFileNameW(&ofn) == TRUE)
    {
        result = std::wstring(buffer.data());
    }
    else
    {
        const DWORD errorCode = CommDlgExtendedError();
        if (errorCode != 0 && errorCode != USER_CANCEL_DIALOG_ERROR)
        {
            std::cerr << "Open file dialog error: " << ErrorFormater::GetErrorString(errorCode) << '\n';
        }
    }
    return result;
}

std::vector<UCHAR> FileManager::ReadFileContent(const std::wstring& filePath, std::string& outErrorMessage)
{
    std::vector<UCHAR> buffer;

    const HandleGuard file(CreateFileW(
        filePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NO_CHILD_DESCRIPTOR_INHERITANCE,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    ));

    if (file.get() == INVALID_HANDLE_VALUE)
    {
        outErrorMessage = "Failed to open file: " + ErrorFormater::GetLastErrorString();
        return buffer;
    }

    LARGE_INTEGER fileSizeLi;
    if (!GetFileSizeEx(file.get(), &fileSizeLi))
    {
        outErrorMessage = "Failed to get file size: " + ErrorFormater::GetLastErrorString();
        return buffer;
    }

    if (fileSizeLi.QuadPart <= 0)
        return buffer;

    if (fileSizeLi.QuadPart > MAXDWORD)
    {
        outErrorMessage = "File too large for single read operation";
        return buffer;
    }

    const auto fileSize = static_cast<size_t>(fileSizeLi.QuadPart);
    buffer.resize(fileSize);

    DWORD bytesRead = 0;
    const BOOL readResult = ReadFile(
        file.get(),
        buffer.data(),
        static_cast<DWORD>(fileSize),
        &bytesRead,
        nullptr
    );

    if (!readResult || bytesRead != fileSize)
    {
        outErrorMessage = "Failed to read file: " + ErrorFormater::GetLastErrorString();
        buffer.clear();
    }

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

FileManager::FileSaveResult FileManager::SaveFile(HWND hwnd, const std::string& content, const SaveEncoding encoding)
{
    const std::optional<std::wstring> filePath = SaveFileDialog(hwnd);
    if (!filePath.has_value())
    {
        return {"No file selected", false};
    }
    return SaveFile(content, filePath.value(), encoding);
}

FileManager::FileSaveResult FileManager::SaveFile(const std::string &content, const std::wstring &filePath, const SaveEncoding encoding)
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

std::optional<std::wstring> FileManager::SaveFileDialog(HWND hwnd)
{
    std::vector<WCHAR> buffer(MAX_FILE_PATH, L'\0');

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = buffer.data();
    ofn.nMaxFile = static_cast<DWORD>(buffer.size());
    ofn.lpstrFilter = L"All Files\0*.*\0Text Files\0*.txt\0UTF-8 Files\0*.txt\0UTF-16 Files\0*.txt\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;

    std::optional<std::wstring> result = std::nullopt;
    if (GetSaveFileNameW(&ofn) == TRUE)
    {
        result = std::wstring(buffer.data());
    }
    else
    {
        const DWORD errorCode = CommDlgExtendedError();
        if (errorCode != 0 && errorCode != USER_CANCEL_DIALOG_ERROR)
        {
            std::cerr << "Save file dialog error: " << ErrorFormater::GetErrorString(errorCode) << '\n';
        }
    }

    return result;
}

bool FileManager::WriteFileContent(const std::wstring& filePath, const std::vector<UCHAR>& buffer, std::string& errorMessage)
{
    HandleGuard file(CreateFileW(
        filePath.c_str(),
        GENERIC_WRITE,
         NO_SHARING_FILE_TO_OTHER_PROCESS_UNTIL_CLOSE,
        NO_CHILD_DESCRIPTOR_INHERITANCE,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        nullptr
    ));

    if (file.get() == INVALID_HANDLE_VALUE)
    {
        errorMessage = "Failed to create file: " + ErrorFormater::GetLastErrorString();
        return false;
    }

    bool success = true;
    size_t totalBytesWritten = 0;
    const size_t bufferSize = buffer.size();

    while (success && totalBytesWritten < bufferSize)
    {
        const size_t remaining = bufferSize - totalBytesWritten;
        const size_t chunkSize = (remaining > READ_CHUNK_SIZE) ? READ_CHUNK_SIZE : remaining;

        DWORD bytesWritten = 0;
        const BOOL writeResult = WriteFile(
            file.get(),
            buffer.data() + totalBytesWritten,
            static_cast<DWORD>(chunkSize),
            &bytesWritten,
            nullptr
        );

        if (!writeResult)
        {
            errorMessage = "WriteFile failed: " + ErrorFormater::GetLastErrorString();
            success = false;
        }
        else if (bytesWritten != chunkSize)
        {
            errorMessage = "WriteFile wrote "     + std::to_string(bytesWritten) +
                          " bytes instead of "    + std::to_string(chunkSize) +
                          " (disk full?), total=" + std::to_string(totalBytesWritten);
            success = false;
        }
        else
        {
            totalBytesWritten += bytesWritten;
        }
    }

    if (success)
    {
        if (!FlushFileBuffers(file.get()))
        {
            errorMessage = "Failed to flush file buffers: " + ErrorFormater::GetLastErrorString();
            success = false;
        }
    }

    if (!success)
    {
        file.close();
        const WINBOOL result = DeleteFileW(filePath.c_str());
        if(result != TRUE)
        {
            std::cout << ErrorFormater::GetLastErrorString() << '\n';
        }
    }

    return success;
}

std::string FileManager::ConvertWStringToStdString(const std::wstring& content)
{
    if (content.empty())
        return {};

    const int bufferSize = WideCharToMultiByte(
        CP_UTF8,
        0,
        content.c_str(),
        CALCULATE_NULL_TERMINATED_STRING,
        NO_OUTPUT_BUFFER,
        ZERO_MULTI_BYTE,
        nullptr,
        nullptr
    );

    if (bufferSize == 0)
    {
        return {};
    }

    std::vector<CHAR> buffer(bufferSize);

    const int result = WideCharToMultiByte(
        CP_UTF8,
        0,
        content.c_str(),
        CALCULATE_NULL_TERMINATED_STRING,
        buffer.data(),
        bufferSize,
        nullptr,
        nullptr
    );

    if (result == 0)
    {
        return {};
    }

    std::string resultStr = std::string(buffer.data(), buffer.size() - 1);
    return resultStr;
}
