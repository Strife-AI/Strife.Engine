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
        prev->next = next;
        next->prev = prev;

        next = nullptr;
        prev = nullptr;
    }

    DLinkedNode<TElement>* next = nullptr;
    DLinkedNode<TElement>* prev = nullptr;
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

    T* operator*()
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
    DLinkedList(const DLinkedList& rhs) = delete;

    DLinkedListIterator<T> begin()
    {
        return DLinkedListIterator<T>(head.next);
    }

    DLinkedListIterator<T> end()
    {
        return DLinkedListIterator<T>(&tail);
    }

    void Append(Node* node);
    void InsertBefore(Node* node, Node* before);

    Node head;
    Node tail;
};

template<typename T>
void DLinkedList<T>::Append(DLinkedList::Node* node)
{
    InsertBefore(node, &tail);
}

template<typename T>
void DLinkedList<T>::InsertBefore(Node* node, Node* before)
{
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
