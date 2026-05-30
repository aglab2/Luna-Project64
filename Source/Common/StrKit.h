#pragma once

// !!! You must always consider cases where you can _avoid_ converting strings in the first place. !!!
// Prefer std::string to std::wstring where possible, Windows 10 is capable of proper UTF-8

#include <string>

std::wstring ToWide(const std::string& text);
std::string ToUtf8(const std::wstring& text);
