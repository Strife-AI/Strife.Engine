#pragma once

#include <typeinfo>

#include "System/Logger.hpp"

// Stores a list of free TItems
// Notes:
//		* This class does not allocate any memory. You must preallocate your own items.
//		* The size of TItem must be big enough to hold a pointer.
template <typename TItem>
class FreeList
{
public:
    FreeList()
    {
        // Make sure the node is big enough to hold the next pointer
        static_assert(sizeof(TItem) >= sizeof(TItem*), "Item is not big enough to hold a pointer");
    }

    FreeList(TItem initialItems[], int count)
    {
        for (int i = 0; i < count; ++i)
        {
            Return(&initialItems[i]);
        }
    }

    TItem* Borrow();
    void Return(TItem* item);
    bool HasFreeItem() const
    {
        return _head != nullptr;
    }

    void Clear()
    {
        _head = nullptr;
    }

private:
    struct FreeListNode
    {
        TItem* GetItem()
        {
            return reinterpret_cast<TItem*>(this);
        }

        static FreeListNode* FromItem(TItem* item)
        {
            return reinterpret_cast<FreeListNode*>(item);
        }

        FreeListNode* next;
    };

    FreeListNode* _head = nullptr;
};

template <typename TItem>
TItem* FreeList<TItem>::Borrow()
{
    if (_head == nullptr)
    {
        FatalError("Out of items in free list");
    }
    FreeListNode* node = _head;
    _head = _head->next;

    return node->GetItem();
}

template <typename TItem>
void FreeList<TItem>::Return(TItem* item)
{
    if (item == nullptr)
    {
        FatalError("Tried to return nullptr to free list");
    }

    FreeListNode* node = FreeListNode::FromItem(item);

    if (_head == nullptr)
    {
        node->next = nullptr;
        _head = node;
    }
    else
    {
        node->next = _head;
        _head = node;
    }
}
