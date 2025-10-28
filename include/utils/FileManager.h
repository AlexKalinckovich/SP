#pragma once
#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <windows.h>
#include <string>
#include <optional>
#include <memory>
#include <vector>
#include "EncodingDetector.h" // Assumed to exist

// Forward declarations
struct FileLoadResult;
enum class SaveEncoding; // Assumed to exist

#define NO_TEMPORARY_FILE nullptr
#define INVALID_FILE_HANDLE INVALID_HANDLE_VALUE
#define READ_CHUNK_SIZE 4096
#define MAX_PATH_LENGTH 32767
#define NO_SHARING_FILE_TO_OTHER_PROCESS_UNTIL_CLOSE 0
#define NO_CHILD_DESCRIPTOR_INHERITANCE nullptr
#define FILE_MAPPING_READ_ACCESS PAGE_READONLY
#define FILE_MAPPING_VIEW_READ_ACCESS FILE_MAP_READ
#define HIGH_DWORD_OF_OFFSET_VIEW 0
#define LOW_DWORD_OF_OFFSET_VIEW 0
#define TO_THE_END_OF_FILE 0

class FileManager
{
public:
    struct FileLoadResult
    {
        std::vector<UCHAR> buffer; // Used for sample/small file
        std::wstring filePath;
        Encoding encoding = Encoding::UNKNOWN;
        bool isSuccess = false;
        size_t fileSize = 0;
    };

    struct FileViewInfo
    {
        const char* data;
        size_t size;
        size_t offset;
    };

    // --- Memory Mapped File Reader ---
    class MemoryMappedReader
    {
    public:
        MemoryMappedReader();
        ~MemoryMappedReader() noexcept;

        bool OpenFile(const std::wstring& filePath);
        void CloseFile() noexcept;

        [[nodiscard]] FileViewInfo GetView(size_t offset, size_t size) const;
        [[nodiscard]] size_t GetFileSize() const noexcept { return m_fileSize; }
        [[nodiscard]] bool IsFileOpen() const noexcept { return m_fileMapping != INVALID_FILE_HANDLE; }

        [[nodiscard]] std::string GetTextChunk(size_t offset, size_t maxChars, Encoding encoding) const;

    private:
        HANDLE m_fileHandle;
        HANDLE m_fileMapping;
        size_t m_fileSize;
        const char* m_mappedData;
    };

    // This function gets file metadata and a sample for encoding detection
    static bool LoadFileForMapping(HWND hwnd, FileLoadResult& result);
    static bool LoadFileForMapping(const std::wstring& filePath, FileLoadResult& result);

    // For small files
    static bool LoadFile(HWND hwnd, FileLoadResult& fileLoadResult);
    static bool LoadFile(const std::wstring &filePath, FileLoadResult &result);

    // Saving
    static bool SaveFile(HWND hwnd, const std::string& content);
    static bool SaveFile(const std::wstring& filePath, const std::string& content);

    static std::optional<std::wstring> OpenFileDialog(HWND hwnd);

private:
    struct HandleGuard
    {
        HANDLE h;
        explicit HandleGuard(HANDLE handle = INVALID_HANDLE_VALUE) : h(handle) {}
        ~HandleGuard() { if (h != INVALID_HANDLE_VALUE) CloseHandle(h); }
        [[nodiscard]] HANDLE get() const { return h; }
    };

    static std::vector<UCHAR> ReadFileContent(const std::wstring &filePath);
    static bool ProcessFileContent(const std::vector<UCHAR> &buffer, FileLoadResult &result);

    static void SetErrorFileLoadResult(FileLoadResult& result);
    static void SetEmptyFileLoadResult(FileLoadResult& result);
};

#endif // FILEMANAGER_H

