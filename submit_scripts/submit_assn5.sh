#!/bin/bash
#
#SBATCH --cpus-per-task=1
#SBATCH --nodes=1
#SBATCH --ntasks=4
#SBATCH --partition=slow
#SBATCH --mem=10G

python /scratch/assignment5/test_scripts/triangle_counting_tester.pyc --execPath=/home/cta106/CMPT431/assignments/assignment5/triangle_counting_parallel
# python /scratch/assignment5/test_scripts/page_rank_tester.pyc --execPath=/home/cta106/CMPT431/assignments/assignment5/page_rank_parallel
# python /scratch/assignment5/test_scripts/submission_validator.pyc --tarPath=/home/cta106/CMPT431/assignments/assignment5/assignment5.tar.gz