#pragma once

#include <string>
#include <vector>

namespace NumConv
{
    static inline void justify(std::string& str, long length, char fill)
    {
        size_t size = str.length();
        if (size == length) return;

        bool right = length >= 0;
        length = abs(length);

        if (size < length) {  //expand
            str.resize(length);
            char* p = str.data();
            size_t displacement = length - size;
            if (right) memmove(p + displacement, p, size);
            else p += size;
            while (displacement--) *p++ = fill;
        }
        else
        {  //shrink
            char* p = str.data();
            size_t displacement = size - length;
            if (right) memmove(p, p + displacement, length);
            str.resize(length);
        }
    }

    template<typename T>
    static inline std::string hex(T value, long precision = 0, char padchar = '0') {
        std::string buffer;
        buffer.resize(sizeof(T) * 2);
        char* p = buffer.data();

        //create a mask to clear the upper four bits after shifting right in case T is a signed type
        T mask = 1;
        mask <<= sizeof(T) * 8 - 4;
        mask -= 1;

        uint32_t size = 0;
        do {
            uint32_t n = value & 15;
            p[size++] = n < 10 ? '0' + n : 'a' + n - 10;
            value = value >> 4 & mask;
        } while (value);
        buffer.resize(size);
        std::reverse(buffer.begin(), buffer.end());
        if (precision) justify(buffer, precision, padchar);
        return buffer;
    }

    static inline uint64_t toBinary_(const char* s, size_t left, uint64_t sum = 0)
    {
        if (0 == left)
            return 0;

        return (
            *s == '0' || *s == '1' ? toBinary_(s + 1, left - 1, (sum << 1) | *s - '0') :
            *s == '\'' ? toBinary_(s + 1, left - 1, sum) :
            sum
            );
    }

    static inline uint64_t toOctal_(const char* s, size_t left, uint64_t sum = 0)
    {
        if (0 == left)
            return 0;

        return (
            *s >= '0' && *s <= '7' ? toOctal_(s + 1, left - 1, (sum << 3) | *s - '0') :
            *s == '\'' ? toOctal_(s + 1, left - 1, sum) :
            sum
            );
    }

    static inline uint64_t toDecimal_(const char* s, size_t left, uint64_t sum = 0)
    {
        if (0 == left)
            return 0;

        return (
            *s >= '0' && *s <= '9' ? toDecimal_(s + 1, left - 1, (sum * 10) + *s - '0') :
            *s == '\'' ? toDecimal_(s + 1, left - 1, sum) :
            sum
            );
    }

    static inline uint64_t toHex_(const char* s, size_t left, uint64_t sum = 0)
    {
        return (
            !left ? sum :
            *s >= 'A' && *s <= 'F' ? toHex_(s + 1, left - 1, (sum << 4) | *s - 'A' + 10) :
            *s >= 'a' && *s <= 'f' ? toHex_(s + 1, left - 1, (sum << 4) | *s - 'a' + 10) :
            *s >= '0' && *s <= '9' ? toHex_(s + 1, left - 1, (sum << 4) | *s - '0') :
            *s == '\'' ? toHex_(s + 1, left - 1, sum) :
            sum
            );
    }

    static inline uint64_t hex(const std::string_view& sv)
    {
        size_t left = sv.length();
        if (0 == left)
            return 0;

        const char* s = sv.data();
        return (
            *s == '0' && left > 1 && (*(s + 1) == 'X' || *(s + 1) == 'x') ? toHex_(s + 2, left - 2) :
            *s == '$' ? toHex_(s + 1, left - 1) : toHex_(s, left)
            );
    }

    static inline uint64_t toNatural(const char* s, size_t left)
    {
        if (0 == left)
            return 0;

        return (
            *s == '0' && left > 1 && (*(s + 1) == 'B' || *(s + 1) == 'b') ? toBinary_(s + 2, left - 2) :
            *s == '0' && left > 1 && (*(s + 1) == 'O' || *(s + 1) == 'o') ? toOctal_(s + 2, left - 2) :
            *s == '0' && left > 1 && (*(s + 1) == 'X' || *(s + 1) == 'x') ? toHex_(s + 2, left - 2) :
            *s == '%' ? toBinary_(s + 1, left - 1) : *s == '$' ? toHex_(s + 1, left - 1) : toDecimal_(s, left)
            );
    }

    static inline int64_t integer(const std::string_view& sv) {
        size_t left = sv.length();
        if (0 == left)
            return 0;

        const char* s = sv.data();
        return (
            *s == '+' ? +toNatural(s + 1, left - 1) : *s == '-' ? -toNatural(s + 1, left - 1) : toNatural(s, left)
            );
    }
    
    static inline void hexByte(char* out, uint8_t value)
    {
        out[0] = "0123456789ABCDEF"[value >> 4];
        out[1] = "0123456789ABCDEF"[value & 0xF];
    }

    static inline std::string hex(const std::vector<uint8_t>& bytes)
    {
        std::string res;
        res.resize(bytes.size() * 2);
        for (int i = 0; i < bytes.size(); i++)
        {
            hexByte(res.data() + 2 * i, bytes[i]);
        }

        return res;
    }
}
