#!/bin/bash
#
#SBATCH --cpus-per-task=8
#SBATCH --time=10:00
#SBATCH --mem=2G
#SBATCH --partition=fast

# for i in {1..10}
# do
#     echo "Run number: $i"

#     srun /home/cta106/CMPT431/assignments/assignment3/non_blocking_queue_throughput --n_producers 4 --n_consumers 4 --seconds 5 --init_allocator 100000000
#     srun /home/cta106/CMPT431/assignments/assignment3/two_lock_queue_throughput --n_producers 4 --n_consumers 4 --seconds 5 --init_allocator 100000000
#     srun /home/cta106/CMPT431/assignments/assignment3/one_lock_queue_throughput --n_producers 4 --n_consumers 4 --seconds 5 --init_allocator 100000000
# done

# Array of producer/consumer counts
# producer_consumer_counts=(1 2 4)

# for count in "${producer_consumer_counts[@]}"
# do
#     echo "Testing with $count producers and $count consumers"
#     for i in {1..10}
#     do
#         echo "Run number: $i for $count producers/consumers"

#         srun /home/cta106/CMPT431/assignments/assignment3/non_blocking_queue_throughput --n_producers $count --n_consumers $count --seconds 5 --init_allocator 100000000
#         srun /home/cta106/CMPT431/assignments/assignment3/two_lock_queue_throughput --n_producers $count --n_consumers $count --seconds 5 --init_allocator 100000000
#         srun /home/cta106/CMPT431/assignments/assignment3/one_lock_queue_throughput --n_producers $count --n_consumers $count --seconds 5 --init_allocator 100000000
#     done
# done

for i in {1..10}
do
    echo "Run number: $i"

    srun /home/cta106/CMPT431/assignments/assignment3/one_lock_stack_throughput --n_producers 4 --n_consumers 4 --seconds 5 --init_allocator 100000000
    srun /home/cta106/CMPT431/assignments/assignment3/lock_free_stack_throughput --n_producers 4 --n_consumers 4 --seconds 5 --init_allocator 100000000
done

# ### throughput
# srun /home/cta106/CMPT431/assignments/assignment3/non_blocking_queue_throughput --n_producers 4 --n_consumers 4 --seconds 5 --init_allocator 100000000
# srun /home/cta106/CMPT431/assignments/assignment3/two_lock_queue_throughput --n_producers 4 --n_consumers 4 --seconds 5 --init_allocator 100000000
# srun /home/cta106/CMPT431/assignments/assignment3/one_lock_queue_throughput --n_producers 4 --n_consumers 4 --seconds 5 --init_allocator 100000000

# srun /home/cta106/CMPT431/assignments/assignment3/one_lock_stack_throughput --n_producers 4 --n_consumers 4 --seconds 5 --init_allocator 100000000
# srun /home/cta106/CMPT431/assignments/assignment3/lock_free_stack_throughput --n_producers 4 --n_consumers 4 --seconds 5 --init_allocator 100000000

### correctness
# srun /home/cta106/CMPT431/assignments/assignment3/one_lock_queue_correctness --n_producers 2 --n_consumers 2 --input_file /scratch/assignment3/inputs/rand_10M
# srun /home/cta106/CMPT431/assignments/assignment3/two_lock_queue_correctness --n_producers 2 --n_consumers 2 --input_file /scratch/assignment3/inputs/rand_10M
# srun /home/cta106/CMPT431/assignments/assignment3/non_blocking_queue_correctness --n_producers 2 --n_consumers 2 --input_file /scratch/assignment3/inputs/rand_10M

# srun /home/cta106/CMPT431/assignments/assignment3/one_lock_stack_correctness --n_producers 2 --n_consumers 2 --input_file /scratch/assignment3/inputs/rand_10M
# srun /home/cta106/CMPT431/assignments/assignment3/lock_free_stack_correctness --n_producers 2 --n_consumers 2 --input_file /scratch/assignment3/inputs/rand_10M


# ./one_lock_queue_correctness --n_producers 2 --n_consumers 2 --input_file /scratch/assignment3/inputs/rand_10M
# ./two_lock_queue_correctness 2 --n_consumers 2 --input_file /scratch/assignment3/inputs/rand_10M
# ./non_blocking_queue_correctness 2 --n_consumers 2 --input_file /scratch/assignment3/inputs/rand_10M