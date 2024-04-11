# Advanced MPI Communication for Triangle Counting and PageRank

## Introduction

This assignment builds on Assignment 4, aiming to enhance distributed Triangle Counting and PageRank programs using sophisticated MPI communication methods.

### Pre-requisites

- Completion of the Slurm and MPI tutorials is essential.
- Understand how data layout affects program performance to avoid false sharing.

## General Instructions

- Serial implementations are provided as a starting point.
- Focus on two communication strategies with MPI (`--strategy` value 1 or 2).
- Use a single thread per process. Ensure to call `MPI_Finalize` before exiting.
- Adjust the `--mem` option in your script for a 10GB memory limit.

### Testing and Validation

- Validate your solutions using test scripts found at `/scratch/assignment5/test_scripts/`.
- Ensure `--cpus-per-task=1` in your slurm job script, and set `--ntasks` and `--nodes` accordingly.

### Sample Slurm Script

```bash
#!/bin/bash
#SBATCH --cpus-per-task=1
#SBATCH --nodes=1
#SBATCH --ntasks=4
#SBATCH --partition=slow
#SBATCH --mem=10G

srun ./triangle_counting_parallel
```

## Sample Inputs and Custom Graphs

- Use sample inputs located at `/scratch/input_graphs/` or generate your own using the tool provided at `/scratch/input_graphs/SNAPtoBinary`.

## MPI Implementations

### Triangle Counting with MPI

Implement distributed Triangle Counting with MPI, utilizing edge decomposition for work distribution.

#### Strategies

- **Gather [20 points]:** Use `MPI_Gather()` to collect local counts at the root process.
- **Reduce [15 points]:** Utilize `MPI_Reduce()` with `MPI_SUM` to directly aggregate local counts.

### PageRank with MPI

Implement PageRank using MPI, distributing work using edge decomposition.

#### Strategies

- **Reduce and Scatter [25 points]:** Sum `next_page_rank` using `MPI_Reduce()` and distribute the summed values using `MPI_Scatterv()`.
- **Direct Reduce [25 points]:** Sum `next_page_rank` for vertices allocated to a process using `MPI_Reduce()`.

## Output Format

- Ensure the output includes world size, communication strategy, process rank, edges processed, triangles counted, communication time, and total execution time.
- Implement the specified strategies as `--strategy 1` and `--strategy 2` for both Triangle Counting and PageRank.
