#pragma once

template<typename T>
struct DLinkedList;

template<typename TElement>
struct DLinkedNode
{
    TElement* GetElement() { return static_cast<TElement*>(this); }
    TElement* GetElement() const { return static_cast<const TElement*>(this); }

    void Unlink()
    {
        if (owner != nullptr)
        {
            prev->next = next;
            next->prev = prev;

            next = nullptr;
            prev = nullptr;
        }
    }

    TElement* next;
    TElement* prev;
    DLinkedList<TElement>* owner;
};

template<typename T>
struct DLinkedListIterator
{
    DLinkedListIterator(DLinkedNode<T>* node)
        : current(node)
    {

    }

    bool operator==(const DLinkedListIterator& rhs) const
    {
        return current == rhs.current;
    }

    bool operator!=(const DLinkedListIterator& rhs) const
    {
        return !(*this == rhs);
    }

    T& operator*()
    {
        return current->GetElement();
    }

    DLinkedListIterator operator++()
    {
        current = current->next;
        return DLinkedListIterator(current);
    }

    DLinkedListIterator operator--()
    {
        current = current->prev;
        return DLinkedListIterator();
    }

    DLinkedNode<T>* current;
};

// A doubly-linked list implementation. It's intended to be inherited from. Note that it uses a dummy head and tail node
// to simplify inserting/deleting.
template<typename T>
struct DLinkedList
{
    using Node = DLinkedNode<T>;

    DLinkedList();

    DLinkedListIterator<T> begin()
    {
        return DLinkedListIterator<T>(head.next);
    }

    DLinkedListIterator<T> end()
    {
        return DLinkedListIterator<T>(&tail);
    }

    void Append(Node* node);
    void Remove(Node* node);
    void InsertBefore(Node* node, Node* before);

    Node head;
    Node tail;
};

template<typename T>
void DLinkedList<T>::Append(DLinkedList::Node* node)
{
    node->owner = this;
    InsertBefore(&tail);
}

template<typename T>
void DLinkedList<T>::Remove(DLinkedList::Node* node)
{
    assert(node->owner == this);
}

template<typename T>
void DLinkedList<T>::InsertBefore(Node* node, Node* before)
{
    assert(node->owner == nullptr);

    before->prev->next = node;
    node->prev = before->prev;

    node->next = before;
    before->prev = node;
}

template<typename T>
DLinkedList<T>::DLinkedList()
{
    head.next = &tail;
    head.prev = nullptr;

    tail.next = nullptr;
    tail.prev = &head;
}
