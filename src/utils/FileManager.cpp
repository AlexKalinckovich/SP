#include "utils/FileManager.h"
#include "meta_info/error_codes.h"
#include <commdlg.h>
#include <iostream>
#include <string>
#include <memory>
#include <algorithm>

#include "utils/ErrorFormater.h" // Assumed to exist

#define NO_CHILD_DESCRIPTOR_INHERITANCE nullptr
#define MAX_FILE_PATH 32767
#define USER_CANCEL_DIALOG_ERROR 0
#define CALCULATE_NULL_TERMINATED_STRING (-1)
#define NO_OUTPUT_BUFFER nullptr
#define ZERO_MULTI_BYTE 0

// --- MemoryMappedReader Implementation ---

FileManager::MemoryMappedReader::MemoryMappedReader()
    : m_fileHandle(INVALID_FILE_HANDLE)
    , m_fileMapping(INVALID_FILE_HANDLE)
    , m_fileSize(0)
    , m_mappedData(nullptr)
{
}

FileManager::MemoryMappedReader::~MemoryMappedReader() noexcept
{
    CloseFile();
}

bool FileManager::MemoryMappedReader::OpenFile(const std::wstring& filePath)
{
    ErrorFormatter::ClearError();
    CloseFile();

    m_fileHandle = CreateFileW(
        filePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NO_CHILD_DESCRIPTOR_INHERITANCE,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NO_TEMPORARY_FILE
    );

    if (m_fileHandle == INVALID_FILE_HANDLE)
    {
        return false;
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(m_fileHandle, &fileSize))
    {
        CloseFile();
        return false;
    }

    if (fileSize.QuadPart == 0)
    {
        ErrorFormatter::SetLastApplicationError(U_ERROR_FILE_EMPTY);
        CloseFile();
        return false;
    }

    m_fileSize = static_cast<size_t>(fileSize.QuadPart);

    m_fileMapping = CreateFileMappingW(
        m_fileHandle,
        NO_CHILD_DESCRIPTOR_INHERITANCE,
        FILE_MAPPING_READ_ACCESS,
        HIGH_DWORD_OF_OFFSET_VIEW,
        LOW_DWORD_OF_OFFSET_VIEW,
        NO_TEMPORARY_FILE
    );

    if (m_fileMapping == INVALID_FILE_HANDLE)
    {
        CloseFile();
        return false;
    }

    m_mappedData = static_cast<const char*>(MapViewOfFile(
        m_fileMapping,
        FILE_MAPPING_VIEW_READ_ACCESS,
        HIGH_DWORD_OF_OFFSET_VIEW,
        LOW_DWORD_OF_OFFSET_VIEW,
        TO_THE_END_OF_FILE
    ));

    if (m_mappedData == nullptr)
    {
        CloseFile();
        return false;
    }

    return true;
}

void FileManager::MemoryMappedReader::CloseFile() noexcept
{
    if (m_mappedData)
    {
        UnmapViewOfFile(m_mappedData);
        m_mappedData = nullptr;
    }
    if (m_fileMapping != INVALID_FILE_HANDLE)
    {
        CloseHandle(m_fileMapping);
        m_fileMapping = INVALID_FILE_HANDLE;
    }
    if (m_fileHandle != INVALID_FILE_HANDLE)
    {
        CloseHandle(m_fileHandle);
        m_fileHandle = INVALID_FILE_HANDLE;
    }
    m_fileSize = 0;
}

FileManager::FileViewInfo FileManager::MemoryMappedReader::GetView(const size_t offset, const size_t size) const
{
    if (!IsFileOpen() || offset >= m_fileSize)
    {
        return { nullptr, 0, 0 };
    }
    const size_t availableSize = std::min(size, m_fileSize - offset);
    return { m_mappedData + offset, availableSize, offset };
}

std::string FileManager::MemoryMappedReader::GetTextChunk(const size_t offset, const size_t maxChars, Encoding encoding) const
{
    FileViewInfo view = GetView(offset, maxChars);
    if (view.data == nullptr || view.size == 0)
    {
        return "";
    }

    std::vector<UCHAR> buffer(view.data, view.data + view.size);
    return EncodingDetector::ConvertToUTF8(buffer, encoding);
}


// --- FileManager Static Methods ---

bool FileManager::LoadFileForMapping(HWND hwnd, FileLoadResult& result)
{
    ErrorFormatter::ClearError();
    const std::optional<std::wstring> filepath = OpenFileDialog(hwnd);

    if (!filepath.has_value())
    {
        result.isSuccess = false;
        SetEmptyFileLoadResult(result);
        return false; // Error set by OpenFileDialog
    }

    return LoadFileForMapping(filepath.value(), result);
}

bool FileManager::LoadFileForMapping(const std::wstring& filePath, FileLoadResult& result)
{
    ErrorFormatter::ClearError();
    result.filePath = filePath;

    const HandleGuard file(CreateFileW(
        filePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NO_CHILD_DESCRIPTOR_INHERITANCE,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NO_TEMPORARY_FILE
    ));

    if (file.get() == INVALID_HANDLE_VALUE)
    {
        result.isSuccess = false;
        SetErrorFileLoadResult(result);
        return false;
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(file.get(), &fileSize))
    {
        result.isSuccess = false;
        SetErrorFileLoadResult(result);
        return false;
    }

    result.fileSize = static_cast<size_t>(fileSize.QuadPart);

    if (result.fileSize == 0)
    {
        ErrorFormatter::SetLastApplicationError(U_ERROR_FILE_EMPTY);
        result.isSuccess = true; // Empty file is a "successful" load
        result.encoding = Encoding::UNKNOWN;
        result.buffer.clear();
        return true;
    }

    // Read a sample for encoding detection
    std::vector<UCHAR> sampleBuffer(std::min(static_cast<size_t>(READ_CHUNK_SIZE), result.fileSize));
    DWORD bytesRead = 0;

    if (ReadFile(file.get(), sampleBuffer.data(), static_cast<DWORD>(sampleBuffer.size()), &bytesRead, nullptr) && bytesRead > 0)
    {
        result.encoding = EncodingDetector::Detect(sampleBuffer.data(), bytesRead);
        result.isSuccess = true;
        // We don't store the buffer, LoadFileForMapping is just for metadata
        sampleBuffer.clear();
    }
    else
    {
        result.isSuccess = false;
        SetErrorFileLoadResult(result);
    }

    return result.isSuccess;
}

bool FileManager::LoadFile(HWND hwnd, FileLoadResult& fileLoadResult)
{
    ErrorFormatter::ClearError();
    const std::optional<std::wstring> filepath = OpenFileDialog(hwnd);

    bool success = false;
    if (!filepath.has_value())
    {
        SetEmptyFileLoadResult(fileLoadResult);
    }
    else
    {
        success = LoadFile(filepath.value(), fileLoadResult);
    }
    return success;
}

bool FileManager::LoadFile(const std::wstring& filePath, FileLoadResult& result)
{
    ErrorFormatter::ClearError();
    const std::vector<UCHAR> buffer = ReadFileContent(filePath);

    bool success = false;
    // Check GetLastError because ReadFileContent sets it on failure
    if (buffer.empty() && GetLastError() != 0)
    {
        result.filePath = filePath;
        SetErrorFileLoadResult(result);
    }
    else
    {
        success = ProcessFileContent(buffer, result);
        result.filePath = filePath;
    }
    return success;
}

std::vector<UCHAR> FileManager::ReadFileContent(const std::wstring &filePath)
{
    ErrorFormatter::ClearError();
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
        return buffer;
    }

    LARGE_INTEGER fileSizeLi;
    if (!GetFileSizeEx(file.get(), &fileSizeLi))
    {
        return buffer;
    }

    if (fileSizeLi.QuadPart <= 0)
    {
        ErrorFormatter::SetLastApplicationError(U_ERROR_FILE_EMPTY);
        return buffer;
    }

    if (fileSizeLi.QuadPart > MAXDWORD)
    {
        ErrorFormatter::SetLastApplicationError(U_ERROR_FILE_TOO_LARGE);
        return buffer;
    }

    const size_t fileSize = static_cast<size_t>(fileSizeLi.QuadPart);
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
        buffer.clear();
        return buffer;
    }

    return buffer;
}

bool FileManager::ProcessFileContent(const std::vector<UCHAR>& buffer, FileLoadResult& result)
{
    bool success = false;

    if (!buffer.empty())
    {
        const Encoding encoding = EncodingDetector::Detect(buffer.data(), buffer.size());
        result.buffer = buffer;
        result.encoding = encoding;
        result.fileSize = buffer.size();
        success = true;
    }

    if (!success)
    {
        SetErrorFileLoadResult(result);
        ErrorFormatter::SetLastApplicationError(U_ERROR_FILE_EMPTY);
    }

    return success;
}

void FileManager::SetErrorFileLoadResult(FileLoadResult& result)
{
    result.buffer.clear();
    result.encoding = Encoding::UNKNOWN;
    result.fileSize = 0;
    result.isSuccess = false;
}

void FileManager::SetEmptyFileLoadResult(FileLoadResult& result)
{
    result.buffer.clear();
    result.encoding = Encoding::UNKNOWN;
    result.fileSize = 0;
    result.filePath.clear();
    result.isSuccess = false;
}

std::optional<std::wstring> FileManager::OpenFileDialog(HWND hwnd)
{
    ErrorFormatter::ClearError();
    std::vector<wchar_t> buffer(MAX_FILE_PATH, L'\0');

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
        if (errorCode == 0)
        {
            ErrorFormatter::SetLastApplicationError(U_ERROR_USER_CANCELLED);
        }
    }
    return result;
}

bool FileManager::SaveFile(const std::wstring& filePath, const std::string& content)
{
    ErrorFormatter::ClearError();

    const HandleGuard file(CreateFileW(
        filePath.c_str(),
        GENERIC_WRITE,
        0,
        NO_CHILD_DESCRIPTOR_INHERITANCE,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    ));

    if (file.get() == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    DWORD bytesWritten = 0;
    const BOOL writeResult = WriteFile(
        file.get(),
        content.data(),
        static_cast<DWORD>(content.size()),
        &bytesWritten,
        nullptr
    );

    return writeResult && (bytesWritten == content.size());
}

bool FileManager::SaveFile(HWND hwnd, const std::string& content)
{
    ErrorFormatter::ClearError();
    std::vector<wchar_t> buffer(MAX_FILE_PATH, L'\0');

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = buffer.data();
    ofn.nMaxFile = static_cast<DWORD>(buffer.size());
    ofn.lpstrFilter = L"All Files\0*.*\0Text Files\0*.txt\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileNameW(&ofn) == TRUE)
    {
        return SaveFile(std::wstring(buffer.data()), content);
    }

    return false;
}

