#include "../common/allocator.h"
#include "mutex"

template <class T>
class Node
{
public:
    T value;
    Node<T> *next;
};

template <class T>
class OneLockStack
{
    Node<T> *top;
    std::mutex stack_mtx_lock;
    CustomAllocator my_allocator_;
public:
    OneLockStack() : my_allocator_()
    {
        std::cout << "Using OneLockStack\n";
    }

    void initStack(long t_my_allocator_size)
    {
        std::cout << "Using Allocator\n";
        my_allocator_.initialize(t_my_allocator_size, sizeof(Node<T>));
        Node<T> *new_node = (Node<T> *)my_allocator_.newNode();
        if (!new_node)
        {
            std::cout << "Error in allocating memory\n";
            exit(EXIT_FAILURE);
        }

        new_node->next = nullptr;
        top = new_node;
        my_allocator_.freeNode(new_node);
        // Perform any necessary initializations
    }

    /**
     * Create a new node with value `value` and update it to be the top of the stack.
     * This operation must always succeed.
     */
    void push(T value)
    {
        stack_mtx_lock.lock();
        Node<T> *node = (Node<T> *)my_allocator_.newNode();
        node->value = value;
        node->next = top->next;
        top->next = node;
        
        stack_mtx_lock.unlock();
        my_allocator_.freeNode(node);
    }

    /**
     * If the stack is empty: return false.
     * Otherwise: copy the value at the top of the stack into `value`, update
     * the top to point to the next element in the stack, and return true.
     */
    bool pop(T *value)
    {
        stack_mtx_lock.lock();
        Node<T> *new_top = top->next;

        if (new_top == nullptr)
        {
            // Stack is empty
            stack_mtx_lock.unlock();
            return false;
        }

        *value = new_top->value;
        top = new_top;

        stack_mtx_lock.unlock();
        my_allocator_.freeNode(new_top);
        return true;
    }

    void cleanup()
    {
        my_allocator_.cleanup();
    }
};
