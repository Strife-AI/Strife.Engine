#pragma once

#include <initializer_list>

template<typename TEnum>
struct Flags
{
	Flags() = default;
	constexpr Flags(const std::initializer_list<TEnum>& flags)
	{
		for(auto flag : flags) this->flags |= static_cast<unsigned int>(flag);
	}

    bool HasFlag(TEnum flag) const
    {
        return (flags & static_cast<unsigned int>(flag)) != 0;
    }

    void SetFlag(TEnum flag)
    {
        flags |= static_cast<unsigned int>(flag);
    }

    void ResetFlag(TEnum flag)
	{
    	flags &= ~static_cast<unsigned int>(flag);
	}

    unsigned int flags = 0;
};