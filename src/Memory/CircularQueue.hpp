#pragma once

#include "System/Logger.hpp"

template<typename T>
class CircularQueueIterator
{
public:
    CircularQueueIterator(T* begin_, T* current_, int capacity)
        : begin(begin_),
          current(current_),
          _capacity(capacity)
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
        current = current + 1 == begin + _capacity
            ? begin
            : current + 1;

        return CircularQueueIterator(begin, current, _capacity);
    }

    CircularQueueIterator operator--()
    {
        current = current == begin
            ? current + _capacity - 1
            : current - 1;

        return CircularQueueIterator(begin, current, _capacity);
    }

private:
    T* begin;
    T* current;
    int _capacity;
};

template<typename T>
class CircularQueue
{
public:
    CircularQueue()
        : head(nullptr),
        tail(nullptr),
        items(nullptr),
        capacity(0)
    {

    }

    CircularQueue(T* items, int capacity)
        : head(items),
        tail(items),
        items(items),
        capacity(capacity)
    {

    }

    void SetStorage(T* items, int capacity)
    {
        head = tail = this->items = items;
        this->capacity = capacity;
    }

    int Capacity() const { return capacity; }

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

    void Enqueue(T&& item)
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

    T* DequeueHeadIfFullAndAllocate()
    {
        if (IsFull())
        {
            Dequeue();
        }

        return Allocate();
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

    CircularQueueIterator<T> begin()
    {
        return CircularQueueIterator<T>(items, head, capacity);
    }

    CircularQueueIterator<T> end()
    {
        return CircularQueueIterator<T>(items, tail, capacity);
    }

    void Clear()
    {
        head = tail = items;
    }

    T& Last()
    {
        return *(--end());
    }

private:
    T* Next(T* ptr)
    {
        return ptr + 1 == items + capacity
            ? items
            : ptr + 1;
    }

    T* head;
    T* tail;
    T* items;
    int capacity;
    int count = 0;
};

template<typename T, int QueueCapacity>
class FixedSizeCircularQueue : public CircularQueue<T>
{
public:
    FixedSizeCircularQueue()
        : CircularQueue<T>(_data, QueueCapacity)
    {

    }

    static constexpr int Capacity() { return QueueCapacity; }

private:
    T _data[QueueCapacity];
};