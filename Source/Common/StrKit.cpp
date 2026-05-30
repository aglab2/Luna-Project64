#include "StrKit.h"

#include <windows.h>

std::wstring ToWide(const std::string& text)
{
    if (text.empty())
    {
        return std::wstring();
    }

    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (sizeNeeded <= 0)
    {
        return std::wstring();
    }

    std::wstring wideText((size_t)sizeNeeded, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wideText[0], sizeNeeded);

    if (!wideText.empty() && wideText.back() == L'\0')
    {
        wideText.pop_back();
    }
    return wideText;
}

std::string ToUtf8(const std::wstring& text)
{
    if (text.empty())
    {
        return {};
    }

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 0)
    {
        return {};
    }

    std::string utf8Text((size_t)sizeNeeded, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &utf8Text[0], sizeNeeded, nullptr, nullptr);

    if (!utf8Text.empty() && utf8Text.back() == '\0')
    {
        utf8Text.pop_back();
    }
    return utf8Text;
}
