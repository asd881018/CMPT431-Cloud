Assignment 3 [35 + 22 Points]
In this assignment, we will implement and evaluate concurrent queues and stacks. We will look at two lock-based queues, one lockless queue, one lock-based stack, and a lockless stack. Your task is to implement and observe the performance of these data structures, and write a report that answers the specific questions listed here Assignment 3 - Report (submission guidelines available at the bottom of this page).

Before starting this assignment, you should have completed the Slurm Tutorial which walks you through how to use our servers for your code development.

Performance of a program is often sensitive to the way in which data is laid out in memory. Tutorial 2 explains how data layout in memory can affect performance. You should ensure your solutions do not suffer from false sharing, and are not limited by poor choice of data layout.

General Instructions
Read this paper: Simple, Fast, and Practical Non-Blocking and Blocking Concurrent Queue Algorithms. We will implement and evaluate the queues proposed in this paper.
Download the assignment code (queue/stack interfaces, driver programs, etc.) here.
The queue interfaces are provided to you in following files:
queues/one_lock_queue.h
queues/two_lock_queue.h
queues/non_blocking_queue.h
The stack interfaces are provided to you in following files:
stacks/one_lock_stack.h
stacks/lock_free_stack.h
You are also given two driver programs to evaluate the correctness and performance of your queue/stack implementations. You should not modify these files for your submission.
driver_correctness.cpp - In this driver program, producers repeatedly enqueue/push elements from an input file into the queue/stack and consumers repeatedly dequeue/pop elements from the queue/stack. The driver program then validates whether the enqueued/pushed and dequeued/popped elements are correct. It supports the following command-line parameters:

--n_producers: Number of producer threads that will enqueue/push to the queue/stack.
--n_consumers: Number of consumer threads that will dequeue/pop from the queue/stack.
--input_file: Absolute path of the input file that contains the elements to be inserted.
--init_allocator: Initial number of elements to be pre-allocated. We will rely on pre-allocation to eliminate the effects of allocation/deallocation on throughput numbers (more on this in next point).
For example, to test correctness of one_lock_queue with 2 producers and 2 consumers, run:

$ make one_lock_queue_correctness
$ ./one_lock_queue_correctness --n_producers 2 --n_consumers 2 --input_file /scratch/assignment3/inputs/rand_10M
driver_throughput.cpp - In this driver program, producers repeatedly enqueue/push elements in the queue/stack and consumers repeatedly dequeue/pop elements from the queue/stack for a specified period of time. The number of operations performed by the workers is used to measure throughput of the queue/stack. It supports the following command-line parameters:

--seconds: Number of seconds the producer and consumer should run.
--n_producers: Number of producer threads that will enqueue/push to the queue/stack.
--n_consumers: Number of consumer threads that will dequeue/pop from the queue/stack.
--init_allocator: Initial number of elements to be pre-allocated. We will rely on pre-allocation to eliminate the effects of allocation/deallocation on throughput numbers (more on this in next point).
For example, to measure the throughput of non_blocking_queue with 2 producers and 2 consumers for 5 seconds, run:

$ make non_blocking_queue_throughput
$ ./non_blocking_queue_throughput --n_producers 2 --n_consumers 2 --seconds 5 --init_allocator 100000000
We will use a custom allocator for allocating and deallocating elements in the queue/stack. The custom allocator is thread-safe, and hence, you don't need to worry about synchronizing allocation/deallocation calls. The custom allocator can be used as follows:
    CustomAllocator my_allocator;
    // You can initialize the allocator with the number of elements to be pre-allocated and the size of each element
    my_allocator.initialize(1, sizeof(Node<int>));  // Here, 1 element of sizeof(Node<int>) is pre-allocated
    
    // Creating a node using the allocator
    // NOTE: Must typecast explicitly to the desired type as shown below
    Node<int> *new_node = (Node<int>*)my_allocator.newNode();
    // Always set the member variables of the element as the custom allocator does not invoke any constructor.
    new_node.value = 5;
    new_node.next = nullptr;
    
    // Creating another node will cause failure as my_allocator is initialized with only 1 node
    Node<int> *new_node2 = (Node<int>*)my_allocator.newNode();     // This will fail.
    
    // Freeing the allocated node
    my_allocator.freeNode(new_node);
    
    // Cleanup the custom allocator
    my_allocator.cleanup();
The queue/stack interfaces provided to you have the custom allocator object along with the appropriate initialize() and cleanup() calls. Ensure that you use the correct template type for the custom allocator object depending on the node type used by your queue implementation.
Sample inputs are available at /scratch/assignment3/inputs/. These inputs contain a list of items to be inserted in the queue/stack.
You can generate your own dataset by running:
$ /scratch/assignment3/inputs/generate_test_input --nValues 1000 --outputFile "rand_1000"
We have compiled a list of common mistakes and poor parallel programming practices that should be avoided. You are expected to understand them and avoid such mistakes.
1. OneLockQueue: Coarse-Grained Synchronization [5 Points]
For OneLockQueue, we will use a single lock to synchronize across all queue operations. The pseudocode of this solution (without locks) is shown below:

template<class T>
struct Node {
    T value
    Node<T>* next
}

template<class T>
struct OneLockQueue {
    Node<T>* q_head
    Node<T>* q_tail
    CustomAllocator my_allocator
    
    initQueue(allocator_init) {
        my_allocator.initialize(allocator_init, sizeof(Node<T>))
        node = (Node<T>* )my_allocator.newNode()
        node->next = NULL
        q_head = q_tail = node
    }

    enqueue(T value) {
        node = (Node<T>* )my_allocator.newNode()
        node->value = value
        node->next = NULL
        Append to q_tail and update the queue
    }

    dequeue(T* p_value) {
        node = q_head
        new_head = q_head->next
        if(new_head == NULL){
            // Queue is empty
            return FALSE
        }
        *p_value = new_head->value
        Update q_head
        my_allocator.freeNode(node)
        return TRUE
    }

    cleanup() {
        my_allocator.cleanup();
    }
}
Your task is to implement OneLockQueue using a single lock (coarse grained synchronization). You can assume that initQueue() and cleanup() will be called by a single thread only.

2. TwoLockQueue: Fine-Grained Synchronization [5 Points]
For TwoLockQueue, we will use two separate locks for enqueue and dequeue operations. These locks will ensure that at most one enqueuer and one dequeuer can proceed simultaneously. We will use the two-lock queue algorithm proposed by Maged M. Michael and Michael L. Scott (see Figure 2 in paper). The pseudocode for this is the same as OneLockQueue but with separate locks for enqueue and dequeue.

    enqueue(T value) {
        // Use enqueue/tail lock
        // Perform enqueue similar to OneLockQueue 
    }
    dequeue(T* p_value) {
        // Use dequeue/head lock
        // Perform dequeue similar to OneLockQueue
    }
3. Non-Blocking Queue (Michael-Scott Queue) [10 Points]
We will implement the non-blocking queue algorithm proposed by Maged M. Michael and Michael L. Scott (see Figure 1 in paper). The detailed algorithm is shown below.

template<class P>
struct pointer_t {
    P* ptr

    P* address(){
        Get the address by getting the 48 least significant bits of ptr
    }
    uint count(){
        Get the count from the 16 most significant bits of ptr
    }
}

template<class T>
struct Node {
    T value
    pointer_t<Node<T>> next
}

template<class T>
struct NonBlockingQueue {
    pointer_t<Node<T>> q_head
    pointer_t<Node<T>> q_tail
    CustomAllocator my_allocator
    
    initQueue(allocator_init) {
        my_allocator.initialize(allocator_init, sizeof(Node<T>))
        node = (Node<T>* )my_allocator.newNode()
        node->next.ptr = NULL
        q_head.ptr = q_tail.ptr = node
    }

    enqueue(T value) {
        node = (Node<T>* )my_allocator.newNode()
        node->value = value
        node->next.ptr = NULL
        SFENCE;
        while(true) {
            tail = q_tail
            LFENCE;
            next = tail.address()->next
            LFENCE;
            if (tail == q_tail){
                if (next.address() == NULL) {
                    if(CAS(&tail.address()->next, next, <node, next.count()+1>))
                        break
                }
                else
                    CAS(&q_tail, tail, <next.address(), tail.count()+1>)	// ELABEL
            }
        }
        SFENCE;
        CAS(&q_tail, tail, <node, tail.count()+1>)
    }

    dequeue(T* p_value) {
        while(true){
            head = q_head
            LFENCE;
            tail = q_tail
            LFENCE;
            next = head.address()->next
            LFENCE;
            if (head == q_head) {
                if(head.address() == tail.address()) {
                    if(next.address() == NULL)
                            return FALSE;
                    CAS(&q_tail, tail, <next.address(), tail.count()+1>)	//DLABEL
                }
                else {
                    *p_value = next.address()->value
                    if(CAS(&q_head, head, <next.address(), head.count()+1>))
                        break
                }
            }
        }
        my_allocator.freeNode(head.address())
        return TRUE
    }
    cleanup() {
        my_allocator.cleanup();
    }
}
Key things to note in the above algorithm:

The structure of pointer only has ptr as its member variable. Modern 64-bit processors currently support 48-bit virtual addressing. So count is co-located within this same ptr variable such that the most significant 16 bits are used to store the count. This strategy enables us to update both the address and the count in a single CAS operation.
Notice the use of SFENCE and LFENCE in the above algorithm. These are assembly instructions defined as follows:
#define LFENCE asm volatile("lfence" : : : "memory")
#define SFENCE asm volatile("sfence" : : : "memory")
We need to use these fence instructions to ensure that instructions are not reordered by the compiler or the CPU. For example, the three instructions (head = q_head , tail = q_tail , next = head.address()->next) in the below pseudocode need to happen in the exact given order and we ensure this by inserting lfence between them:
head = q_head
LFENCE;
tail = q_tail
LFENCE;
next = head.address()->next
LFENCE;
LFENCE - Ensures that load instructions are not reordered across the fence instruction.

SFENCE - Ensures that store instructions are not reordered across the fence instruction.

Read more about fence instructions here. Note that you don't have to insert any other fence instructions apart from the ones that are explicitly shown in the provided pseudocode.

You can implement the non-blocking queue with or without std::atomic variables. In case you'd like to work without std::atomic, you can use the CAS() function provided in common/utils.h.
4. OneLockStack: Coarse-Grained Synchronization [5 Points]
We will implement an unbounded total stack using a single lock to synchronize across all stack operations. This is similar to OneLockQueue, except the data structure will be stack instead of queue. The key operations are: push(), pop(), initStack() and cleanup(). The API is shown below.

template<class T>
struct OneLockStack {
    void initStack(allocator_init);

    /**
     * Create a new node with value `value` and update it to be the top of the stack.
     * This operation must always succeed.
     */
    void push(T value);

    /**
     * If the stack is empty: return false.
     * Otherwise: copy the value at the top of the stack into `p_value`, update
     * the top to point to the next element in the stack, and return true.
     */
    bool pop(T* p_value);

    void cleanup();
}
Your implementation must use linked-list as the concrete representation (similar to figures on slide 70 in Concurrent Queues & Stacks), and the single lock will be acquired for every push and pop operation. When the stack is not empty, the top pointer points to the next element to be popped. When the stack is empty, you have the choice to either have top pointed to a sentinel node or have top be NULL. The pseudocode for this solution will not be provided, but you should learn from and build using the OneLockQueue pseudo-code provided above.

5. Lock-Free Stack [10 Points]
We will implement the lock-free unbounded total stack. Our solution logic will be similar to the one shown on slide 84 in Concurrent Queues & Stacks where CAS operations directly attempt to modify the top. Specifically, each push operation and each sucessful pop operation requires exactly 1 successful CAS operation. You are not expected to implement the backoff logic (i.e., you can skip the backoff logic completely).

Since the logic in slides is not C++ based, you will have to figure out important implementation details like how to deal with pointers and identify whether stack is empty or not, where to invoke CAS, etc. You should learn from and build using the NonBlockingQueue pseudo-code provided above.

Key things to note in the above algorithm:

You can implement the lock-free stack with or without std::atomic variables. In case you'd like to work without std::atomic, you can use the CAS() function provided in common/utils.h.
Submission Guidelines
Make sure that your solutions folder has the following files and sub-folders. Let's say your solutions folder is called my_assignment3_solutions. It should contain:
common/ -- The folder containing all common files. It is already available in the assignment 3 package. Do not modify it or remove any files.
lib/ -- The folder containing the library. It is already available in the assignment 3 package. Do not modify it or remove any files.
queues/one_lock_queue.h
queues/two_lock_queue.h
queues/non_blocking_queue.h
stacks/one_lock_stack.h
stacks/lock_free_stack.h
Makefile -- Makefile for the project. Do not modify this file.
driver_correctness.cpp -- You are already provided this file. Do not change it.
driver_throughput.cpp -- You are already provided this file. Do not change it.
Adding extra timing/profiling code will affect performance. Ensure there is no additional timing/profiling code included in your final submission.
To create the submission file, follow the steps below:
Enter in your solutions folder, and remove all the object/temporary files.
$ cd my_assignment3_solutions/
$ make clean
Create the tar.gz file.
$ tar cvzf assignment3.tar.gz *
which creates a compressed tar ball that contains the contents of the folder.
Validate the tar ball using the submission_validator.pyc script.
$ python /scratch/assignment3/test_scripts/submission_validator.pyc --tarPath=assignment3.tar.gz
For assignment report,
Create a copy of Assignment 3 - Report.
Fill in your answers.
Select File -> Download -> PDF Document. Save the downloaded file as report.pdf.
Submit via CourSys by the deadline posted there.
Copyright © 2023 Keval Vora. All rights reserved.