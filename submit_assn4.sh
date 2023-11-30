#!/bin/bash
#
#SBATCH --cpus-per-task=1
#SBATCH --nodes=1
#SBATCH --ntasks=4
#SBATCH --partition=slow
#SBATCH --mem=10G

# python /scratch/assignment4/test_scripts/triangle_counting_tester.pyc --execPath=/home/cta106/CMPT431/assignments/assignment4/triangle_counting_parallel
# python /scratch/assignment4/test_scripts/page_rank_tester.pyc --execPath=/home/cta106/CMPT431/assignments/assignment4/page_rank_parallel
python /scratch/assignment4/test_scripts/submission_validator.pyc --tarPath=/home/cta106/CMPT431/assignments/assignment4/assignment4.tar.gz