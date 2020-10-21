#pragma once

template<typename TEnum>
struct Flags
{
    bool HasFlag(TEnum flag) const
    {
        return flags & static_cast<unsigned int>(flag) != 0;
    }

    void SetFlag(TEnum flag)
    {
        flags |= static_cast<unsigned int>(flag);
    }

    unsigned int flags = 0;
};