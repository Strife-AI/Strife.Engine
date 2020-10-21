#include "BlockAllocator.hpp"

#include <set>

static int totalLargeRequested = 0;

void* BlockAllocator::Allocate(int size)
{
    int blockId = GetBlockId(size);

    if (blockId == TooBig)
    {
        totalLargeRequested += size;

        auto data = new char[size];
        if (data == nullptr)
        {
            FatalError("Failed to allocate large block of size %d bytes in BlockAllocator (total %d)", size, totalLargeRequested);
        }

        return data;
    }

    void* block;
    if (_blocks[blockId].HasFreeItem())
    {
        block = _blocks[blockId].Borrow();
    }
    else
    {
        block = _stackAllocator.AllocateOrFatalError(_blockSizes[blockId]);
    }

    memset(block, 0, _blockSizes[blockId]);

    return block;
}

void BlockAllocator::Free(void* data, int size)
{
    int blockId = GetBlockId(size);

    if (blockId == TooBig)
    {
        delete[](char*)(data);
        totalLargeRequested -= size;
    }
    else
    {
        _blocks[blockId].Return((Block*)data);
    }
}

BlockAllocator* BlockAllocator::_defaultAllocator;

BlockAllocator* BlockAllocator::GetDefaultAllocator()
{
    return _defaultAllocator;
}

void BlockAllocator::SetDefaultAllocator(BlockAllocator* allocator)
{
    _defaultAllocator = allocator;
}

int BlockAllocator::GetBlockId(int size)
{
    for (int i = 0; i < TotalBlockSizes; ++i)
    {
        if (size < _blockSizes[i])
        {
            return i;
        }
    }

    return TooBig;
}
