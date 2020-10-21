#pragma once

#include <forward_list>

template<typename T>
class Lazy
{
public:
    template <typename ... Args>
    void Initialize(Args&& ... constructorArgs)
    {
        new(_buf) T(std::forward<Args>(constructorArgs) ...);
        _isInitialized = true;
    }

    T* Value()
    {
        return (T*)_buf;
    }

    ~Lazy()
    {
        if(_isInitialized)
        {
            Value()->~T();
        }
    }

private:
    unsigned char _buf[sizeof(T)] { 0 };
    bool _isInitialized = false;
};
