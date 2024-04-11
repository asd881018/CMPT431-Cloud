#include "../common/allocator.h"
#include <mutex>

template <class T>
class Node
{
public:
    T value;
    Node<T>* next;
};

template <class T>
class TwoLockQueue
{
    Node<T>* q_head;
    Node<T>* q_tail;
    CustomAllocator my_allocator_;
    std::mutex mutex_lock_enqueue;
    std::mutex mutex_lock_dequeue;

public:
    TwoLockQueue() : my_allocator_()
    {
        std::cout << "Using TwoLockQueue\n";
    }

    void initQueue(long t_my_allocator_size){
        std::cout << "Using Allocator\n";
        my_allocator_.initialize(t_my_allocator_size, sizeof(Node<T>));

        // Initialize the queue head or tail here
        Node<T>* new_node = (Node<T>*)my_allocator_.newNode();
        if (!new_node){
            std::cout << "Error in allocating memory\n";
            exit(EXIT_FAILURE);
        }
        
        new_node->next = nullptr;
        q_head = new_node;
        q_tail = new_node;
        my_allocator_.freeNode(new_node);
    }

    void enqueue(T value)
    {
        mutex_lock_enqueue.lock();
        Node<T>* node = (Node<T>*)my_allocator_.newNode();
        node->value = value;
        node->next = nullptr;

        q_tail->next = node;
        q_tail = node;
        mutex_lock_enqueue.unlock();
    }

    bool dequeue(T *value)
    {
        mutex_lock_dequeue.lock();

        Node<T>* node = q_head;
        Node<T>* new_head = q_head->next;

        if (new_head == NULL) {
            // Queue is empty
            mutex_lock_dequeue.unlock();
            return false;
        }

        *value = new_head->value;
        q_head = new_head;
        
        // If the queue is now empty after dequeue
        if (q_head == NULL) { 
            q_tail = NULL;
        }
        mutex_lock_dequeue.unlock();
        my_allocator_.freeNode(node);
        
        return true;
    }

    void cleanup()
    {
        my_allocator_.cleanup();
    }
};