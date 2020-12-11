#pragma once

#include <cstdio>
#include <string>


#include "Crc32.hpp"
#define DEBUG_STRINGID

#if !defined(NDEBUG)
#define DEBUG_STRINGID
#endif

class StringId
{
public:
    constexpr StringId()
        : key(0)
#ifdef DEBUG_STRINGID
          ,
          originalString("<default>")
#endif
    {
    }

    explicit constexpr StringId(const std::string_view& str)
        : key(Crc32Constexpr(str.data(), str.data() + str.length()))
#ifdef DEBUG_STRINGID
        ,
        originalString("<from string view>")
#endif
    {
        
    }

    explicit constexpr StringId(const char* str)
        : key(Crc32Constexpr(str))
#ifdef DEBUG_STRINGID
          ,
          originalString(str)
#endif
    {
    }

    constexpr StringId(unsigned int key_)
        : key(key_)
#ifdef DEBUG_STRINGID
          ,
          originalString("<from key>")
#endif
    {
    }

    constexpr operator unsigned int() const
    {
        return key;
    }

    constexpr bool operator==(const StringId& rhs) const
    {
        return key == rhs.key;
    }

    void print(const char* description) const
    {
#ifdef DEBUG_STRINGID
        printf("%s: %s (%d)\n", description, originalString, key);
#else
        printf("%s: %d\n", description, key);
#endif
    }

    const char* ToString() const
    {
#ifdef DEBUG_STRINGID
        return originalString;
#else
        static char buf[10];
        snprintf(buf, sizeof(buf), "%u", key);

        return buf;
#endif
    }

    static StringId fromString(const char* str)
    {
#ifdef DEBUG_STRINGID
        return StringId(Crc32(str), str);
#else
        return StringId(Crc32(str));
#endif
    }

    unsigned int key;

private:
#ifdef DEBUG_STRINGID
    constexpr StringId(unsigned int key_, const char* str)
        : key(key_),
          originalString(str)
    {
    }

    const char* originalString;
#endif
};

constexpr StringId operator ""_sid(const char* str, size_t len)
{
    return StringId(str);
}

#undef DEBUG_STRINGID
