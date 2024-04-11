# Distributed Algorithms with MPI: Triangle Counting and PageRank

## Introduction

This assignment explores developing distributed solutions for Triangle Counting and PageRank using the Message Passing Interface (MPI). MPI, distinct from shared memory models, employs APIs for communication between processes.

### Pre-requisites

- Completion of the Slurm Tutorial for server use and the MPI tutorial for an overview of MPI programming.
- Understanding the impact of memory data layout on performance.

## General Instructions

- Begin with provided serial implementations.
- This assignment focuses on point-to-point communication (`--strategy 1`) in MPI.
- Use `MPI_Finalize` before exiting your main function.
- Adjust `--mem` and `--cpus-per-task` in your Slurm script as needed.

## MPI Model

- Each MPI process operates independently, each with its own memory space.
- Only one thread per process is used in this assignment.
- Accurately configure your Slurm scripts for the intended number of MPI processes and nodes.

### Example Slurm Script

```bash
#!/bin/bash
#SBATCH --cpus-per-task=1
#SBATCH --nodes=1
#SBATCH --ntasks=4
#SBATCH --partition=slow
#SBATCH --mem=10G

srun ./triangle_counting_parallel
```

## Timing Code Sections

Implement timing for critical sections of your code to monitor performance accurately.

## Testing and Validation

- Use test scripts at `/scratch/assignment4/test_scripts/` for validating outputs.
- Sample inputs are located at `/scratch/input_graphs/`. Instructions are provided for generating custom datasets.

## MPI Implementations

### Triangle Counting with MPI - Point-to-Point

Implement distributed Triangle Counting, distributing work among processes. The root process aggregates local counts to a global count.

### PageRank with MPI - Point-to-Point

Implement PageRank using MPI with edge decomposition, ensuring synchronization and aggregation of PageRank values.

## Output Format

Adhere to the specified output format for both Triangle Counting and PageRank, detailing the world size, process rank, edges processed, and total execution time, among other statistics.
