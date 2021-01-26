#pragma once

#include <cassert>

// Note: this class does not call destructors!
template <typename T, int MaxSize>
class FixedSizeVector
{
public:
    FixedSizeVector()
        : _data{},
          _size(0)
    {
    }

    int Capacity() const
    {
        return MaxSize;
    }

    int Size() const
    {
        return _size;
    }

    bool Empty() const
    {
        return _size == 0;
    }

    bool Full() const
    {
        return _size == MaxSize;
    }

    const T& operator[](int index) const
    {
        return _data[index];
    }

    T& operator[](int index)
    {
        return _data[index];
    }

    void PushBack(const T& item)
    {
        assert(_size < MaxSize);
        _data[_size++] = item;
    }

    void InsertBeforeIfRoom(const T& item, int position)
    {
        if (position == _size) PushBackIfRoom(item);
        else if(_size < MaxSize)
        {
            for(int i = Size(); i > position; --i)
            {
                _data[i] = _data[i - 1];
            }

            _data[position] = item;
            ++_size;
        }
    }

    void PushFrontIfRoom(const T& item)
    {
        if (_size < MaxSize)
        {
            for(int i = Size(); i > 0; --i)
            {
                _data[i] = _data[i - 1];
            }

            _data[0] = item;
            ++_size;
        }
    }

    void PushBackIfRoom(const T& item)
    {
        if(_size < MaxSize)
        {
            _data[_size++] = item;
        }
    }

    void PushBackUnique(const T& item);

    void PopBack()
    {
        assert(_size > 0);
        --_size;
    }

    T* begin() const
    {
        return const_cast<T*>(&_data[0]);
    }

    T* end() const
    {
        return const_cast<T*>(&_data[_size]);
    }

    void Clear()
    {
        _size = 0;
    }

    void Resize(int newSize)
    {
        _size = newSize;
    }

    void RemoveSingle(const T& item);

    bool Contains(const T& item) const
    {
        for(int i = 0; i < _size; ++i)
        {
            if(_data[i] == item)
            {
                return true;
            }
        }

        return false;
    }

private:
    T _data[MaxSize];
    int _size;
};

template <typename T, int MaxSize>
void FixedSizeVector<T, MaxSize>::PushBackUnique(const T& item)
{
    for (int i = 0; i < _size; ++i)
    {
        if (_data[i] == item) { return; }
    }

    PushBack(item);
}

template <typename T, int MaxSize>
void FixedSizeVector<T, MaxSize>::RemoveSingle(const T& item)
{
    for (int i = 0; i < _size; ++i)
    {
        if (_data[i] == item)
        {
            for (int j = i + 1; j < _size; ++j)
            {
                _data[j - 1] = _data[j];
            }

            --_size;

            return;
        }
    }
}
