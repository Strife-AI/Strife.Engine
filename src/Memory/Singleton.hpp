#pragma once

template<typename T>
class Singleton
{
public:
    Singleton() = default;
    Singleton(Singleton const&) = delete;

    void operator=(Singleton const&) = delete;

    static T* GetInstance();

private:

};

template<typename T>
T* Singleton<T>::GetInstance()
{
    static T instance;

    return &instance;
}
