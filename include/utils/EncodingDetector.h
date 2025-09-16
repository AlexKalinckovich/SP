//
// Created by brota on 11.09.2025.
//

#ifndef ENCODINGDETECTOR_H
#define ENCODINGDETECTOR_H
#include <string>
#include <vector>

#include "components/AboutDialog.h"

enum class Encoding {
    UNKNOWN,
    UTF8,
    UTF8_BOM,
    UTF16LE,
    UTF16BE,
    ANSI
};

enum class SaveEncoding {
    UTF8,
    UTF8_BOM,
    UTF16_LE,
    ANSI
};

class EncodingDetector {
public:
    static Encoding Detect(const unsigned char* data, size_t size);
    static std::string ConvertToUTF8(const std::vector<unsigned char>& buffer, Encoding encoding);

    static std::vector<UCHAR> ConvertFromUTF8(const std::string &utf8Content, SaveEncoding encoding);

    static std::vector<UCHAR> ConvertUTF8ToUTF16LE(const std::string &utf8Content, bool includeBOM);

    static std::vector<UCHAR> ConvertUTF8ToANSI(const std::string &utf8Content);

private:
    static constexpr unsigned char UTF8_BOM_BYTES[] = {0xEF, 0xBB, 0xBF};
    static constexpr unsigned char UTF16LE_BOM_BYTES[] = {0xFF, 0xFE};
    static constexpr unsigned char UTF16BE_BOM_BYTES[] = {0xFE, 0xFF};
    static constexpr size_t UTF8_BOM_LENGTH = 3;
    static constexpr size_t UTF16_BOM_LENGTH = 2;

    static constexpr unsigned char ASCII_MASK = 0x80;
    static constexpr unsigned char CONTINUATION_BYTE_MASK = 0xC0;
    static constexpr unsigned char CONTINUATION_BYTE_VALUE = 0x80;
    static constexpr unsigned char TWO_BYTE_LEAD_MASK = 0xE0;
    static constexpr unsigned char TWO_BYTE_LEAD_VALUE = 0xC0;
    static constexpr unsigned char THREE_BYTE_LEAD_MASK = 0xF0;
    static constexpr unsigned char THREE_BYTE_LEAD_VALUE = 0xE0;
    static constexpr unsigned char FOUR_BYTE_LEAD_MASK = 0xF8;
    static constexpr unsigned char FOUR_BYTE_LEAD_VALUE = 0xF0;

    static constexpr UINT ANSI_CODEPAGE = CP_ACP;
    static constexpr UINT UTF8_CODEPAGE = CP_UTF8;

    static bool HasUTF8BOM(const unsigned char* data, size_t size);
    static bool HasUTF16LEBOM(const unsigned char* data, size_t size);
    static bool HasUTF16BEBOM(const unsigned char* data, size_t size);
    static bool IsValidUTF8(const unsigned char* data, size_t size);
    static std::string ConvertUTF16LEToUTF8(const std::vector<unsigned char>& buffer);
    static std::string ConvertANSIToUTF8(const std::vector<unsigned char>& buffer);
};
#endif //ENCODINGDETECTOR_H
