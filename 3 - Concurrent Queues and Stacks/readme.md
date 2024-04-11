# Concurrent Queues and Stacks: Implementation and Performance Analysis

## Overview

Explores the implementation and performance evaluation of various concurrent queues and stacks, including lock-based and lockless designs. Your objective is to implement these data structures, benchmark their performance, and analyze the outcomes in a detailed report.

### Pre-requisites

- Completion of the Slurm Tutorial is necessary for effective use of our development servers.
- Familiarity with the impact of data layout in memory on program performance, as discussed in Tutorial 2.

## General Instructions

- Begin by reviewing the paper: *Simple, Fast, and Practical Non-Blocking and Blocking Concurrent Queue Algorithms*.
- Download the assignment code package, which includes queue/stack interfaces and driver programs.
- Implement the queue and stack interfaces provided:
  - Queues: `one_lock_queue.h`, `two_lock_queue.h`, `non_blocking_queue.h`
  - Stacks: `one_lock_stack.h`, `lock_free_stack.h`
- Do not modify the driver programs (`driver_correctness.cpp` and `driver_throughput.cpp`).

## Driver Programs

### Correctness Testing

- Utilize `driver_correctness.cpp` to test the correctness of your implementations.
- Command-line parameters include:
  - `--n_producers`, `--n_consumers`: Specify the number of producer/consumer threads.
  - `--input_file`: Path to the input file with elements to insert.
  - `--init_allocator`: Pre-allocate elements to avoid allocation/deallocation overhead.

### Throughput Measurement

- Use `driver_throughput.cpp` to measure the throughput of your data structures.
- Parameters include `--seconds` (duration of the test), along with `--n_producers`, `--n_consumers`, and `--init_allocator`.

## Data Structures to Implement

1. **OneLockQueue**: Coarse-grained synchronization with a single lock.
2. **TwoLockQueue**: Fine-grained synchronization with separate enqueue and dequeue locks.
3. **Non-Blocking Queue**: Implementation of the Michael-Scott non-blocking queue algorithm.
4. **OneLockStack**: A single lock stack with coarse-grained synchronization.
5. **Lock-Free Stack**: Implement a lock-free stack without backoff logic.

## Custom Allocator

- Utilize the provided `CustomAllocator` for element allocation and deallocation.
- Example usage and initialization details are provided in the assignment instructions.

## Testing and Inputs

- Sample inputs can be found at `/scratch/assignment3/inputs/`.
- Generate custom datasets using the provided script: `generate_test_input`.
