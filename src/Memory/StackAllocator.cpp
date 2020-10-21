#include "StackAllocator.hpp"
#include "System/Logger.hpp"

#include <cstdlib>


StackAllocator::StackAllocator(int bufSize)
{
    _nextFree = 0;
    _buf = (unsigned char*)malloc(bufSize);

    if (!_buf)
    {
        FatalError("Failed to allocate %d bytes for stack allocator", bufSize);
    }

    _bufSize = bufSize;
}

StackAllocator::~StackAllocator()
{
    free(_buf);
}

bool StackAllocator::TryAllocate(int size, void*& outPtr)
{
    // Align to 32 byte boundary so we fit in a cache line
    _nextFree = (_nextFree + 31) & ~31;

    if (_nextFree + size > _bufSize)
    {
        return false;
    }
    else
    {
        outPtr = (void*)(_buf + _nextFree);
        _nextFree += size;

        return true;
    }
}

void* StackAllocator::AllocateOrFatalError(int size)
{
    void* data;
    if(TryAllocate(size, data))
    {
        return data;
    }
    else
    {
        FatalError("Failed to allocate %d bytes in stack allocator", size);
    }
}

void StackAllocator::Reset()
{
    _nextFree = 0;
}
