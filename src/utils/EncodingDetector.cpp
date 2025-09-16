#include "utils/EncodingDetector.h"
#include <vector>
#include <stdexcept>
#include <cassert>
#include "components/AboutDialog.h"


bool EncodingDetector::HasUTF8BOM(const unsigned char* data, size_t size) {
    return size >= UTF8_BOM_LENGTH &&
           memcmp(data, UTF8_BOM_BYTES, UTF8_BOM_LENGTH) == 0;
}

bool EncodingDetector::HasUTF16LEBOM(const unsigned char* data, size_t size) {
    return size >= UTF16_BOM_LENGTH &&
           memcmp(data, UTF16LE_BOM_BYTES, UTF16_BOM_LENGTH) == 0;
}

bool EncodingDetector::HasUTF16BEBOM(const unsigned char* data, size_t size) {
    return size >= UTF16_BOM_LENGTH &&
           memcmp(data, UTF16BE_BOM_BYTES, UTF16_BOM_LENGTH) == 0;
}

bool EncodingDetector::IsValidUTF8(const unsigned char* data, const size_t size)
{
    size_t i = 0;
    while (i < size)
    {
        if ((data[i] & ASCII_MASK) == 0)
        {
            i++;
        }
        else if ((data[i] & TWO_BYTE_LEAD_MASK) == TWO_BYTE_LEAD_VALUE)
        {
            if (i + 1 >= size || (data[i + 1] & CONTINUATION_BYTE_MASK) != CONTINUATION_BYTE_VALUE)
            {
                return false;
            }
            i += 2;
        }
        else if ((data[i] & THREE_BYTE_LEAD_MASK) == THREE_BYTE_LEAD_VALUE)
        {
            if (i + 2 >= size ||
                (data[i + 1] & CONTINUATION_BYTE_MASK) != CONTINUATION_BYTE_VALUE ||
                (data[i + 2] & CONTINUATION_BYTE_MASK) != CONTINUATION_BYTE_VALUE)
            {
                return false;
            }
            i += 3;
        }
        else if ((data[i] & FOUR_BYTE_LEAD_MASK) == FOUR_BYTE_LEAD_VALUE)
        {
            if (i + 3 >= size ||
                (data[i + 1] & CONTINUATION_BYTE_MASK) != CONTINUATION_BYTE_VALUE ||
                (data[i + 2] & CONTINUATION_BYTE_MASK) != CONTINUATION_BYTE_VALUE ||
                (data[i + 3] & CONTINUATION_BYTE_MASK) != CONTINUATION_BYTE_VALUE)
            {
                return false;
            }
            i += 4;
        } else
        {
            return false;
        }
    }
    return true;
}

Encoding EncodingDetector::Detect(const unsigned char* data, size_t size)
{
    Encoding detectedEncoding = Encoding::ANSI;

    if (HasUTF8BOM(data, size))
    {
        detectedEncoding = Encoding::UTF8_BOM;
    }
    else if (HasUTF16LEBOM(data, size))
    {
        detectedEncoding = Encoding::UTF16LE;
    }
    else if (HasUTF16BEBOM(data, size))
    {
        detectedEncoding = Encoding::UTF16BE;
    }
    else if (IsValidUTF8(data, size))
    {
        detectedEncoding = Encoding::UTF8;
    }

    return detectedEncoding;
}

std::string EncodingDetector::ConvertUTF16LEToUTF8(const std::vector<UCHAR>& buffer)
{
    if (buffer.size() < 2)
    {
        return "";
    }

    const auto *wideStr = reinterpret_cast<const wchar_t*>(buffer.data());
    size_t wideLength = buffer.size() / sizeof(wchar_t);

    if (wideLength >= 1 && wideStr[0] == 0xFEFF)
    {
        wideStr++;
        wideLength--;
    }

    if (wideLength == 0)
    {
        return "";
    }

    const int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wideStr, static_cast<int>(wideLength),
                                               nullptr, 0, nullptr, nullptr);
    if (utf8Length == 0)
    {
        return "";
    }

    std::string result(utf8Length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wideStr, static_cast<int>(wideLength),
                        &result[0], utf8Length, nullptr, nullptr);
    return result;
}

std::string EncodingDetector::ConvertANSIToUTF8(const std::vector<UCHAR>& buffer)
{
    if (buffer.empty()) return "";

    const int wideLength = MultiByteToWideChar(CP_ACP, 0,
                                               reinterpret_cast<const char *>(buffer.data()),
                                               static_cast<int>(buffer.size()),
                                               nullptr, 0);
    if (wideLength == 0)
    {
        return "";
    }

    std::wstring wideStr(wideLength, L'\0');
    MultiByteToWideChar(CP_ACP, 0,
                        reinterpret_cast<const char *>(buffer.data()),
                        static_cast<int>(buffer.size()),
                        &wideStr[0], wideLength);

    const int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), wideLength,
                                               nullptr, 0, nullptr, nullptr);
    if (utf8Length == 0)
    {
        return "";
    }

    std::string result(utf8Length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), wideLength,
                        &result[0], utf8Length, nullptr, nullptr);
    return result;
}

std::string EncodingDetector::ConvertToUTF8(const std::vector<unsigned char>& buffer, const Encoding encoding)
{
    std::string result;

    switch (encoding)
    {
        case Encoding::UTF8:
            result = std::string(buffer.begin(), buffer.end());
            break;
        case Encoding::UTF8_BOM:
            result = std::string(buffer.begin() + UTF8_BOM_LENGTH, buffer.end());
            break;
        case Encoding::UTF16LE:
            result = ConvertUTF16LEToUTF8(buffer);
            break;
        case Encoding::ANSI:
            result = ConvertANSIToUTF8(buffer);
            break;
        default:
            throw std::runtime_error("Unsupported encoding");
    }

    return result;
}

std::vector<UCHAR> EncodingDetector::ConvertFromUTF8(const std::string& utf8Content, SaveEncoding encoding)
{
    if (utf8Content.empty())
    {
        return {};
    }
    std::vector<UCHAR> result;
    switch (encoding)
    {
        case SaveEncoding::UTF8:
            result = std::vector<unsigned char>(utf8Content.begin(), utf8Content.end());
            break;
        case SaveEncoding::UTF8_BOM:
        {
            result.reserve(UTF8_BOM_LENGTH + utf8Content.size());
            result.insert(result.end(), UTF8_BOM_BYTES, UTF8_BOM_BYTES + UTF8_BOM_LENGTH);
            result.insert(result.end(), utf8Content.begin(), utf8Content.end());
            break;
        }

        case SaveEncoding::UTF16_LE:
            result = ConvertUTF8ToUTF16LE(utf8Content, true);
            break;
        case SaveEncoding::ANSI:
            result = ConvertUTF8ToANSI(utf8Content);
            break;
        default:
            result = std::vector<UCHAR>(utf8Content.begin(), utf8Content.end());
    }
    return result;
}

std::vector<unsigned char> EncodingDetector::ConvertUTF8ToUTF16LE(const std::string& utf8Content, const bool includeBOM)
{
    if (utf8Content.empty())
    {
        return includeBOM
                   ? std::vector(UTF16LE_BOM_BYTES, UTF16LE_BOM_BYTES + UTF16_BOM_LENGTH)
                   : std::vector<unsigned char>();
    }

    const int wideLength = MultiByteToWideChar(CP_UTF8, 0, utf8Content.c_str(),
                                               static_cast<int>(utf8Content.size()), nullptr, 0);
    if (wideLength == 0)
    {
        return {};
    }

    std::wstring wideStr(wideLength, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8Content.c_str(),
                        static_cast<int>(utf8Content.size()), &wideStr[0], wideLength);

    size_t resultSize = wideStr.size() * sizeof(wchar_t);
    if (includeBOM)
    {
        resultSize += UTF16_BOM_LENGTH;
    }

    std::vector<UCHAR> result(resultSize);
    size_t offset = 0;

    if (includeBOM)
    {
        result[0] = UTF16LE_BOM_BYTES[0];
        result[1] = UTF16LE_BOM_BYTES[1];
        offset = UTF16_BOM_LENGTH;
    }

    memcpy(result.data() + offset, wideStr.data(), wideStr.size() * sizeof(wchar_t));
    return result;
}

std::vector<UCHAR> EncodingDetector::ConvertUTF8ToANSI(const std::string& utf8Content)
{
    if (utf8Content.empty())
    {
        return {};
    }

    const int wideLength = MultiByteToWideChar(CP_UTF8, 0, utf8Content.c_str(),
                                               static_cast<int>(utf8Content.size()), nullptr, 0);
    if (wideLength == 0)
    {
        return {};
    }

    std::wstring wideStr(wideLength, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8Content.c_str(),
                        static_cast<int>(utf8Content.size()), &wideStr[0], wideLength);

    const int ansiLength = WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), wideLength,
                                               nullptr, 0, nullptr, nullptr);
    if (ansiLength == 0)
    {
        return {};
    }

    std::vector<unsigned char> result(ansiLength);
    WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), wideLength,
                        reinterpret_cast<char *>(result.data()), ansiLength, nullptr, nullptr);

    return result;
}
