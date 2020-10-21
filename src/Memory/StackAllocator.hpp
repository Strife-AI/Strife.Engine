#pragma once

class StackAllocator
{
public:
    StackAllocator(int bufSize);
    ~StackAllocator();

    bool TryAllocate(int size, void*& outPtr);
    void* AllocateOrFatalError(int size);
    void Reset();

private:
    unsigned char* _buf;
    int _bufSize;
    int _nextFree = 0;
};