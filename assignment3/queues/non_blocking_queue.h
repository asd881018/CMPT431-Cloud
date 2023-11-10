#include "../common/allocator.h"

#define LFENCE asm volatile("lfence" : : : "memory")
#define SFENCE asm volatile("sfence" : : : "memory")

template <class P>
struct pointer_t
{
    P *ptr;

    P *address()
    {
        // Get the address by getting the 48 least significant bits of ptr
        return (P *)((uint64_t)ptr & (uint64_t)0x00FFFFFFFFFFFF);
    }

    uint count()
    {
        // Get the count from the 16 most significant bits of ptr
        return (uint)(uint64_t)ptr & (uint64_t)0xFFFF000000000000;
    }

    P *newPointer(P *address, uint count)
    {
        return (P *)((uint64_t)address | ((uint64_t)count & 0xFFFF000000000000));
    }
};

template <class T>
class Node
{
public:
    T value;
    pointer_t<Node<T>> next;
};

template <class T>
class NonBlockingQueue
{
    pointer_t<Node<T>> q_head;
    pointer_t<Node<T>> q_tail;
    CustomAllocator my_allocator_;

public:
    NonBlockingQueue() : my_allocator_()
    {
        std::cout << "Using NonBlockingQueue\n";
    }

    void initQueue(long t_my_allocator_size)
    {
        std::cout << "Using Allocator\n";
        my_allocator_.initialize(t_my_allocator_size, sizeof(Node<T>));
        // Initialize the queue head or tail here
        Node<T> *newNode = (Node<T> *)my_allocator_.newNode();
        if (!newNode)
        {
            std::cout << "Error in allocating memory\n";
            exit(EXIT_FAILURE);
        }
        newNode->next.ptr = nullptr;
        q_head.ptr = newNode;
        q_tail.ptr = newNode;
        my_allocator_.freeNode(newNode);
    }

    void enqueue(T value)
    {
        // Use LFENCE and SFENCE as mentioned in pseudocode
        Node<T> *newNode = (Node<T> *)my_allocator_.newNode();
        newNode->value = value;
        newNode->next.ptr = nullptr;
        pointer_t<Node<T>> tail;
        pointer_t<Node<T>> next;
        pointer_t<Node<T>> node_ptr;

        SFENCE;
        while (true)
        {
            tail = q_tail;
            LFENCE;
            next = tail.address()->next;
            LFENCE;

            if (tail.ptr == q_tail.ptr)
            {
                if (next.address() == NULL)
                {
                    node_ptr.ptr = node_ptr.newPointer(newNode, next.count() + 1);
                    if (CAS(&tail.address()->next, next, node_ptr))
                        break;
                }
                else
                {
                    node_ptr.ptr = node_ptr.newPointer(next.address(), tail.count() + 1);
                    CAS(&q_tail, tail, node_ptr); // ELABEL
                }
            }
        }
        SFENCE;

        node_ptr.ptr = node_ptr.newPointer(newNode, tail.count() + 1);
        CAS(&q_tail, tail, node_ptr);
    }

    bool dequeue(T *value)
    {
        // Use LFENCE and SFENCE as mentioned in pseudocode
        pointer_t<Node<T>> tail;
        pointer_t<Node<T>> head;
        pointer_t<Node<T>> next;
        pointer_t<Node<T>> node_ptr;

        while (true)
        {
            head = q_head;
            LFENCE;
            tail = q_tail;
            LFENCE;
            next = head.address()->next;
            LFENCE;
            if (head.ptr == q_head.ptr)
            {
                if (head.address() == tail.address())
                {
                    if (next.address() == NULL)
                    {
                        return false;
                    }
                    node_ptr.ptr = node_ptr.newPointer(next.address(), tail.count() + 1);
                    CAS(&q_tail, tail, node_ptr); // DLABEL
                }
                else
                {
                    *value = next.address()->value;
                    node_ptr.ptr = node_ptr.newPointer(next.address(), head.count() + 1);
                    if (CAS(&q_head, head, node_ptr))
                    {
                        break;
                    }
                }
            }
        }
        my_allocator_.freeNode(head.address());
        return true;
    }

    void cleanup()
    {
        my_allocator_.cleanup();
    }
};
