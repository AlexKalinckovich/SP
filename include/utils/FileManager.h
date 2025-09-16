//
// Created by brota on 11.09.2025.
//

#ifndef FILEMANAGER_H
#define FILEMANAGER_H
#include <optional>
#include <string>

#include "EncodingDetector.h"

class FileManager
{
    public:
        struct FileLoadResult
        {
            std::wstring filePath;
            std::string content;
            Encoding encoding;
            std::string errorMessage;
            bool isSuccess;
        };

        struct FileSaveResult
        {
            std::string errorMessage;
            bool isSuccess;
        };

        static FileLoadResult LoadFile(HWND hwnd);
        static FileLoadResult LoadFile(const std::wstring& filePath);
        static FileSaveResult SaveFile(HWND hwnd, const std::string& content, SaveEncoding encoding = SaveEncoding::UTF8);
        static FileSaveResult SaveFile(const std::string &content, const std::wstring &filePath, SaveEncoding encoding = SaveEncoding::UTF8);
        static std::string ConvertWStringToStdString(const std::wstring &content);


    private:
        struct HandleGuard
        {
            HANDLE h;
            explicit HandleGuard(HANDLE h_) : h(h_) {}
            ~HandleGuard()
            {
                close();
            }

            void close()
            {
                if (h && h != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(h);
                    h = INVALID_HANDLE_VALUE;
                }
            }

            [[nodiscard]] HANDLE get() const { return h; }
        };
        static constexpr DWORD READ_CHUNK_SIZE = 4096;

        static std::optional<std::wstring> OpenFileDialog(HWND hwnd);
        static std::optional<std::wstring> SaveFileDialog(HWND hwnd);

        static std::vector<UCHAR> ReadFileContent(const std::wstring& filePath, std::string& outErrorMessage);
        static bool WriteFileContent(const std::wstring& filePath, const std::vector<UCHAR>& buffer, std::string& errorMessage);


        static FileLoadResult ProcessFileContent(const std::vector<UCHAR>& buffer, const std::string& errorMessage);
};
#endif //FILEMANAGER_H
