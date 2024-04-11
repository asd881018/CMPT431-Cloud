# Parallel Programming with C++11 Threads

This guide introduces the fundamentals of parallelizing simple programs using the C++11 thread library (`std::thread`). The focus is on converting serial C++ implementations to parallel versions, exploring different parallelization strategies, and understanding the expected outcomes.

## Getting Started

- **Pre-requisites:** Completion of an introductory tutorial on using our servers is required for code development and execution.
- **Performance Considerations:** It's crucial to consider data layout in memory for program performance. Avoid false sharing and ensure optimal data layout to prevent performance bottlenecks.

## General Instructions

- **Serial Programs:** Serial versions of all programs are provided. To compile a program, e.g., `pi_calculation.cpp`, run `make pi_calculation` to create the executable `pi_calculation`.
- **Execution:** Create a slurm job to run the executable with specific command-line arguments. For example:
`./pi_calculation --nPoints 12345678 --nWorkers 4`

- **Testing:** Ensure correct `cpus-per-task` settings in your slurm config. Programs are evaluated based on their output format, matching the provided sample outputs closely.

## Key Concepts and Implementations

### 1. Monte Carlo Pi Estimation

- **Objective:** Estimate the value of Pi using the Monte Carlo method, parallelizing the point generation process across multiple threads.
- **Implementation Highlights:** 
- Each thread generates a subset of points, and the final Pi value is calculated from the aggregate results.
- Parallel implementation should use a different random seed per thread to ensure varied point generation.

### 2. Triangle Counting

- **Objective:** Count the number of triangles in a graph, with each thread processing a sub-graph.
- **Implementation Highlights:** 
- Threads compute triangle counts for their allocated vertices, and results are aggregated to find the total unique triangles.

### 3. PageRank with Locks

- **Objective:** Implement the PageRank algorithm, using locks for synchronizing access to shared resources.
- **Implementation Highlights:** 
- Parallelize computation across threads, ensuring thread-safe operations with `std::mutex`.
- Use barriers to synchronize thread execution between major steps of the algorithm.

### 4. PageRank with Atomics

- **Objective:** Improve the PageRank implementation by eliminating locks in favor of atomic operations.
- **Implementation Highlights:** 
- Utilize `std::atomic` and `compare_exchange` to manage concurrent updates without locks.

## Testing and Submission

- **Testing:** Use provided test scripts to validate your parallel implementations. Ensure compliance with the expected output formats.
- **Submission Package:** Include all necessary source files and Makefile in your submission folder, ensuring no object files are present.
- **Validation and Submission:** Validate your submission package with the provided script and submit as instructed.

## Important Notes

- Access to graph datasets and tools for generating test graphs are provided.
- Pay attention to common pitfalls in parallel programming, avoiding inefficient practices and ensuring correct synchronization.

*Copyright © 2023 Keval Vora. All rights reserved.*
