#!/bin/bash
#
#SBATCH --cpus-per-task=4
#SBATCH --time=02:00
#SBATCH --mem=2G
#SBATCH --partition=slow

# srun /home/$USER/CMPT431/assignments/assignment0/producer_consumer
# srun /home/$USER/CMPT431/assignments/assignment1/pi_calculation --nWorkers 4 --nPoints 1000
# srun python /scratch/assignment0/test_scripts/solution_tester.pyc --execPath=/home/cta106/CMPT431/assignments/assignment0/producer_consumer
srun python /scratch/assignment1/test_scripts/submission_validator.pyc --tarPath=/home/cta106/CMPT431/assignments/assignment1/assignment1.tar.gz
# srun python /scratch/assignment1/test_scripts/pi_calculation_tester.pyc --execPath=/home/cta106/CMPT431/assignments/assignment1/pi_calculation_parallel
# srun python /scratch/assignment1/test_scripts/page_rank_lock_tester.pyc --execPath=/home/cta106/CMPT431/assignments/assignment1/page_rank_parallel
# srun python /scratch/assignment1/test_scripts/page_rank_atomic_tester.pyc --execPath=/home/cta106/CMPT431/assignments/assignment1/page_rank_parallel_atomic