#pragma once

template <typename TLink>
struct DLinkNode
{
    DLinkNode()
        : next(nullptr),
          prev(nullptr)
    {
    }

    void InsertAfterThis(TLink* link)
    {
        link->prev = static_cast<TLink*>(this);
        link->next = next;

        if (next != nullptr)
            next->prev = link;

        next = link;
    }

    void InsertBeforeThis(TLink* link)
    {
        link->next = static_cast<TLink*>(this);
        link->prev = prev;

        if (prev != nullptr)
            prev->next = link;

        prev = link;
    }

    void InsertMeBetween(TLink* prev, TLink* next)
    {
        this->next = next;
        this->prev = prev;

        if (next != nullptr)
            next->prev = static_cast<TLink*>(this);

        if (prev != nullptr)
            prev->next = static_cast<TLink*>(this);
    }

    void Unlink()
    {
        if (prev)
            prev->next = next;

        if (next)
            next->prev = prev;

        next = nullptr;
        prev = nullptr;
    }

    TLink* next;
    TLink* prev;
};
