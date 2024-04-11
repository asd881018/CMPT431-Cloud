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
class LockFreeStack
{
    pointer_t<Node<T>> top;
    CustomAllocator my_allocator_;

public:
    LockFreeStack() : my_allocator_()
    {
        std::cout << "Using LockFreeStack\n";
    }

    void initStack(long t_my_allocator_size)
    {
        std::cout << "Using Allocator\n";
        my_allocator_.initialize(t_my_allocator_size, sizeof(Node<T>));
        // Perform any necessary initializations
        Node<T> *newNode = (Node<T> *)my_allocator_.newNode();
        if (!newNode)
        {
            std::cout << "Error in allocating memory\n";
            exit(EXIT_FAILURE);
        }
        newNode->next.ptr = nullptr;
        top.ptr = newNode;
        my_allocator_.freeNode(newNode);
    }

    /**
     * Create a new node with value `value` and update it to be the top of the stack.
     * This operation must always succeed.
     */
    void push(T value)
    {
        Node<T> *newNode = (Node<T> *)my_allocator_.newNode();
        if (!newNode)
        {
            std::cout << "Error in allocating memory\n";
            exit(EXIT_FAILURE);
        }
        newNode->value = value;
        pointer_t<Node<T>> top_ptr;
        pointer_t<Node<T>> next_ptr;
        pointer_t<Node<T>> node_ptr;
        SFENCE;
        while (true)
        {
            top_ptr = top;
            LFENCE;
            next_ptr = top_ptr.address()->next;
            LFENCE;
            // node_ptr.ptr = top_ptr.ptr;
            // newNode->next.ptr = next_ptr.ptr;
            if (top.ptr == top_ptr.ptr)
            {
                newNode->next.ptr = next_ptr.ptr;
                LFENCE;
                node_ptr.ptr = node_ptr.newPointer(newNode, next_ptr.count() + 1);
                if (CAS(&top_ptr.address()->next, next_ptr, node_ptr))
                {
                    break;
                }
            }
        }
        SFENCE;
    }

    /**
     * If the stack is empty: return false.
     * Otherwise: copy the value at the top of the stack into `value`, update
     * the top to point to the next element in the stack, and return true.
     */
    bool pop(T *value)
    {
        pointer_t<Node<T>> top_ptr;
        pointer_t<Node<T>> next_ptr;
        pointer_t<Node<T>> node_ptr;
        while (true)
        {
            top_ptr = top.ptr->next;
            LFENCE;
            if (top_ptr.ptr == nullptr)
            {
                return false;
            }
            next_ptr = top_ptr.ptr->next;
            LFENCE;
            if (top_ptr.address() == top.address()->next.address())
            {
                *value = top_ptr.address()->value;
                node_ptr.ptr = node_ptr.newPointer(next_ptr.address(), top_ptr.count() + 1);
                if (CAS(&top.ptr->next, top_ptr, node_ptr))
                {
                    break;
                }
            }
        }
        my_allocator_.freeNode(top_ptr.address());
        return true;
    }

    void cleanup()
    {
        my_allocator_.cleanup();
    }
};
