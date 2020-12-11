#pragma once
#include "FreeList.hpp"
#include "StackAllocator.hpp"

class BlockAllocator
{
public:
    BlockAllocator(int bufSize)
        : _stackAllocator(bufSize)
    {
        
    }

    void* Allocate(int size);
    void Free(void* data, int size);

    static BlockAllocator* GetDefaultAllocator();
    static void SetDefaultAllocator(BlockAllocator* allocator);

private:
    static constexpr int _blockSizes[] =
    {
        64,
        128,
        256,
        512,
        1024,
        2048,
        4096
    };

    static constexpr int TotalBlockSizes = sizeof(_blockSizes) / sizeof(int);

    struct Block
    {
        void* next;
    };

    static constexpr int TooBig = -1;
    static int GetBlockId(int size);

    FreeList<Block> _blocks[TotalBlockSizes];
    StackAllocator _stackAllocator;

    static BlockAllocator* _defaultAllocator;
};
