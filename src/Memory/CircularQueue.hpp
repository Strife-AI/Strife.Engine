#pragma once

#include "System/Logger.hpp"

template<typename T, int Size>
class CircularQueueIterator
{
public:
    CircularQueueIterator(T* begin_, T* current_)
        : begin(begin_),
        current(current_)
    {

    }

    bool operator==(const CircularQueueIterator& rhs) const
    {
        return current == rhs.current;
    }

    bool operator!=(const CircularQueueIterator& rhs) const
    {
        return !(*this == rhs);
    }

    T& operator*()
    {
        return *current;
    }

    CircularQueueIterator operator++()
    {
        current = current + 1 == begin + Size
            ? begin
            : current + 1;

        return CircularQueueIterator(begin, current);
    }

private:
    T* begin;
    T* current;
};

template<typename T, int size>
class CircularQueue
{
public:
    CircularQueue()
        : head(items),
        tail(items)
    { }

    bool IsFull()
    {
        return Next(tail) == head;
    }

    bool IsEmpty()
    {
        return head == tail;
    }

    void Enqueue(const T& item)
    {
        *Allocate() = item;
        ++count;
    }

    T* Allocate()
    {
        if (IsFull())
        {
            FatalError("Circular queue full (count %d)", count);
        }

        T* ptr = tail;
        tail = Next(tail);

        return ptr;
    }

    T* Dequeue()
    {
        if (IsEmpty())
        {
            FatalError("Tried to dequeue from empty circular queue");
        }

        T* ptr = head;
        head = Next(head);
        --count;

        return ptr;
    }

    const T& Peek() const
    {
        return *head;
    }

    T& Peek()
    {
        return *head;
    }

    CircularQueueIterator<T, size> begin()
    {
        return CircularQueueIterator<T, size>(items, head);
    }

    CircularQueueIterator<T, size> end()
    {
        return CircularQueueIterator<T, size>(items, tail);
    }

private:
    T* Next(T* ptr)
    {
        return ptr + 1 == items + size
            ? items
            : ptr + 1;
    }

    T items[size];
    T* head;
    T* tail;
    int count = 0;
};