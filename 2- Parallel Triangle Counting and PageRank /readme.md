# Advanced Strategies for Parallel Programming: Triangle Counting and PageRank Improvements

## Overview

Building on the foundations laid in Assignment 1, this assignment delves into refining the parallel solutions for Triangle Counting and PageRank. The focus is on experimenting with various task decomposition and mapping strategies to optimize performance. A critical component of this assignment is analyzing the impact of these strategies and documenting your findings in a comprehensive report.

### Pre-requisites

- Completion of the Slurm Tutorial for server utilization.
- Understanding of memory data layout's impact on performance, as outlined in Tutorial 2.

## General Instructions

- Serial versions of programs and sample outputs are provided.
- Utilize the `--strategy` command-line argument to select between strategies (`1`, `2`, or `3`).
- The `--nWorkers` argument specifies the number of threads (e.g., `--nWorkers 4`).
- Ensure appropriate `cpus-per-task` settings in your slurm config.
- Timing of code regions is crucial for analysis, with detailed instructions provided for measuring specific segments.
- Test scripts are available for preliminary validation, accessible at `/scratch/assignment2/test_scripts/`.

## Task Overview

1. **Triangle Counting Improvements:**
   - **Vertex-based Task Decomposition:** The naive approach, serving as the baseline (`--strategy 1`).
   - **Edge-based Task Decomposition:** Distributes edges across threads for balanced workloads (`--strategy 2`).
   - **Dynamic Task Mapping:** Adopts a flexible work allocation strategy to optimize thread utilization (`--strategy 3`).

2. **PageRank Enhancements:**
   - **Vertex-based Task Decomposition:** Continuation of the atomic-based solution with a focus on vertex allocation (`--strategy 1`).
   - **Edge-based Task Decomposition:** Ensures an equitable distribution of edges to threads (`--strategy 2`).
   - **Dynamic Task Mapping and Granularity:** Introduces a dynamic allocation mechanism with configurable granularity (`--strategy 3`, `--granularity`).


## Important Links and Resources

- Sample inputs and scripts are located on compute nodes under `/scratch/input_graphs/` and `/scratch/assignment2/test_scripts/`, respectively.
- Considerations for memory data layout and common parallel programming pitfalls are integral to your solutions.

*Copyright © 2023 Keval Vora. All rights reserved.*
