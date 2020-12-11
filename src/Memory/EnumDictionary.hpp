#pragma once

// NOTE: the items in the dictionary must be sequential and start at 0!
template<typename TEnum, typename TValue, int TotalItems>
class EnumDictionary
{
public:
    TValue& operator[](TEnum key)
    {
        return _items[static_cast<int>(key)];
    }

    const TValue& operator[](TEnum key) const
    {
        return _items[static_cast<int>(key)];
    }

    TValue& operator[](int index)
    {
        return _items[index];
    }

    const TValue& operator[](int index) const
    {
        return _items[index];
    }


    int GetTotalItems() const { return TotalItems; }

    TValue* begin() { return _items; }
    TValue* end() { return _items + TotalItems; }

private:
    TValue _items[TotalItems];
};