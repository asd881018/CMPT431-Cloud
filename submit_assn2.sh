#!/bin/bash
#
#SBATCH --cpus-per-task=4
#SBATCH --time=02:00
#SBATCH --mem=2G
#SBATCH --partition=slow

#  srun python /scratch/assignment2/test_scripts/triangle_counting_tester.pyc --execPath=/home/cta106/CMPT431/assignments/assignment2/triangle_counting_parallel
srun python /scratch/assignment2/test_scripts/page_rank_tester.pyc --execPath=/home/cta106/CMPT431/assignments/assignment2/page_rank_parallel

 ## ./triangle_counting_parallel --nWorkers 4 --strategy 1 --inputFile /scratch/input_graphs/lj