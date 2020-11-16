#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstring>

template<int MaxLength>
class FixedLengthString
{
public:
    constexpr FixedLengthString()
        : buf{}
    {
        buf[0] = '\0';
    }

    constexpr FixedLengthString(const char* str)
        : buf{}
    {
        CopyFromCString(str);
    }

    constexpr FixedLengthString(const std::string& str)
        : buf{}
    {
        CopyFromCString(str.c_str());
    }

    void Format(const char* formatStr, va_list list)
    {
        vsnprintf(buf, MaxLength, formatStr, list);
    }

    void VFormat(const char* formatStr, ...)
    {
        va_list list;
        va_start(list, formatStr);

        Format(formatStr, list);

        va_end(list);
    }

    bool IsEmpty() const
    {
        return buf[0] == '\0';
    }

    template<int OtherMaxLength>
    void operator=(const FixedLengthString<OtherMaxLength>& str)
    {
        CopyFromCString(str.buf);
    }

    int Capacity() const
    {
        return MaxLength;
    }

    void operator=(const char* str)
    {
        CopyFromCString(str);
    }

    void operator=(const std::string& str)
    {
        CopyFromCString(str.c_str());
    }

    void operator=(const std::string_view& str)
    {
        int copyLength = Min((int)str.length(), MaxLength - 1);
        memcpy(buf, str.data(), copyLength);
        buf[copyLength + 1] = '\0';
    }

    const char& operator[](int index) const
    {
        return buf[index];
    }

    char& operator[](int index)
    {
        return buf[index];
    }

    template<int OtherMaxLength>
    bool operator==(const FixedLengthString<OtherMaxLength>& str)
    {
        return strcmp(buf, str.buf) == 0;
    }

    bool operator==(const char* str) const
    {
        return strcmp(buf, str) == 0;
    }

    template<int OtherMaxLength>
    bool operator!=(const FixedLengthString<OtherMaxLength>& str)
    {
        return !(*this == str);
    }

    bool operator!=(const char* str)
    {
        return !(*this == str);
    }

    static constexpr int GetMaxLength()
    {
        return MaxLength;
    }

    int Length() const
    {
        return strlen(buf);
    }

    void Append(char c)
    {
        int len = Length();
        if(len + 2 < MaxLength)
        {
            buf[len] = c;
            buf[len + 1] = '\0';
        }
    }

    void Clear()
    {
        buf[0] = '\0';
    }

    const char* c_str() const
    {
        return buf;
    }

    std::string_view ToStringView() const
    {
        return std::string_view(buf, buf + Length());
    }

private:
    void CopyFromCString(const char* str)
    {
        strncpy(buf, str, MaxLength);

        // Always append a null terminator just to be safe
        buf[MaxLength - 1] = '\0';
    }

    char buf[MaxLength];
};
