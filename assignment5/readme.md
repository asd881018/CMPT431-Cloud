Assignment 5 [85 points]
In Assignment 4, we developed distributed programs using MPI's point-to-point communication. In this assignment, we will use more sophisticated communication methods to improve our distributed Triangle Counting and PageRank programs.

Before starting this assignment, you should have completed the Slurm Tutorial, which walks you through how to use our servers for your code development. Additionally, you should also have completed the MPI tutorial, which gives an overview of MPI and how to correctly run MPI programs using slurm.

Performance of a program is often sensitive to the way in which data is laid out in memory. Tutorial 2 explains how data layout in memory can affect performance. You should ensure your solutions do not suffer from false sharing, and are not limited by poor choice of data layout.

General Instructions:
You are given the serial implementations here.
MPI permits various communication strategies to pass data between processes which we distinguish using the strategy commandline argument. The value of --strategy will be either 1 or 2 (more details presented with each strategy below).
For simplicity, we only use one thread per process in this assignment. Make sure you use MPI_Finalize before exiting the main() function.
MPI uses the distributed model where each process is completely independent and has its own separate memory space. So, we have temporarily increased the memory limit to 10GB. Remember to set the --mem option appropriately in your script.
While testing your solutions, make sure that --cpus-per-task is set to 1 in your slurm job script, and the --ntasks and --nodes is set based on number of MPI processes and nodes you want.
#!/bin/bash
#
#SBATCH --cpus-per-task=1
#SBATCH --nodes=1
#SBATCH --partition=slow
#SBATCH --ntasks=4
#SBATCH --mem=10G

srun ./triangle_counting_parallel
You will be asked to print the time spent by different threads on specific code regions. The time spent by any code region can be computed as follows:
timer t1;
t1.start();
/* ---- Code region whose time is to be measured --- */
double time_taken = t1.stop();
If you need to time a sub-section inside a loop, you can do that as follows:
double time_taken = 0.0;
timer t1;
while(True){
    /* ---- Code region whose time should not be measured --- */

    t1.start();
    /* ---- Code region whose time is to be measured --- */
    time_taken += t1.stop();

    /* ---- Code region whose time should not be measured --- */
}
std::cout << "Time spent on required code region : " << time_taken << "\n";
We have provided test scripts for you to quickly test your solutions during your development process. You can test your code using the test scripts available at /scratch/assignment5/test_scripts/. Note that these test scripts only validate the output formats, and a different evaluation script will be used for grading the assignments. Important: You should always use slurm when performing these tests. The test scripts under /scratch/assignment5/test_scripts/ folder test for up to 4 processes on the same node; make sure --ntasks=4, --nodes=1, and --partitions=slow is set in your slurm job.
$ ls /scratch/assignment5/test_scripts/*tester.pyc
triangle_counting_tester.pyc page_rank_tester.pyc
Here's an example job script to validate your code:
#!/bin/bash
#
#SBATCH --cpus-per-task=1
#SBATCH --nodes=1
#SBATCH --partition=slow
#SBATCH --ntasks=4
#SBATCH --mem=10G

python /scratch/assignment5/test_scripts/triangle_counting_tester.pyc --execPath=$HOME/my_assignment5_solutions/triangle_counting_parallel
Note that the python script is not invoked with srun. It internally takes care of launching the MPI processes using srun.
Sample input graphs are available at /scratch/input_graphs/.
$ ls /scratch/input_graphs/*.cs*
lj.csc  lj.csr  roadNet-CA.csc  roadNet-CA.csr  rmat.csc  rmat.csr
If you are interested in checking out the original graph datasets (this is not required to solve the assignment), you can find them here.

If you'd like to test your solution with more graph datasets, you can create your own simple graphs as follows:
Create a file called testGraph.txt with the list of edges (one edge on each line) in the graph. For example,
1 2
2 3
Run
$ /scratch/input_graphs/SNAPtoBinary testGraph.txt testGraphConverted
This will create testGraphConverted.csr and testGraphConverted.csc files which are CSR and CSC representations of the graph.
To use the graphs in your solutions, use the command line argument --inputFile "testGraphConverted".
Since each MPI process is independent, use these rules to print your outputs:
Use printf() to avoid garbled logs. You can also concatenate the information as a string and use std::cout to print a single line of output. To add a new line, use "\n" as part of the concatenated string instead of std::endl.
You can check the rank of the process before printing as shown below:
if (world_rank == 0)
    printf("Time taken (in seconds): %g\n", time_taken);
The root process should print most of the output logs.
Non-root processes should only print the process statistics in a single line.
We have compiled a list of common mistakes and poor parallel programming practices that should be avoided. You are expected to understand them and avoid such mistakes.
Triangle Counting using MPI
Similar to Assignment 1, we will develop a parallel solution for Triangle Counting using MPI. Here, the work is distributed among P processes. For simplicity, every process will read the entire input graph and create its own graph data structure. For questions 1 and 2, we will use edge decomposition strategy to distribute the work among the processes (as done in Assignment 2). Use the following pseudocode for determining the subset of vertices for each process:

// Edge decomposition strategy for a graph with n vertices and m edges for P processes
start_vertex=0; end_vertex=0;
for(i=0; i<P; i++){
    start_vertex=end_vertex;
    long count = 0;
    while (end_vertex < n)
    {
        // add vertices until we reach m/P edges.
        count += g.vertices_[end_vertex].getOutDegree();
        end_vertex += 1;
        if (count >= m/P)
            break;
    }
    if(i == global_rank)
        break;
}
// Each process will work on vertices [start_vertex, end_vertex).
Each process will compute the number of triangles for the vertices allocated to it. Process 0 (henceforth, referred as root process) aggregates (i.e., sums up) the local counts from other processes (henceforth, referred as non-root processes) and computes the final triangle count. The pseudocode for questions 1 and 2 is given below:

for each process P in parallel {
    local_count = 0
    for each vertex 'u' allocated to P {
        edges_processed += outDegree(u)  // used in output validation
        for vertex 'v' in outNeighbor(u)
            local_count += count_triangles_for_edge(u, v)
    }
    // --- synchronization phase start ---
    if(P is root process){
        global_count = Sum of local counts of all the processes
    }
    else{
        // depending on the strategy,
        // use appropriate API to send the local_count to the root process
    }
    // --- synchronization phase end -----

    if(P is root process){
        global_count = global_count / 3
        // print process statistics and other results
    }
    else{
        // print process statistics
    }
}
We will use different communication strategies as discussed below.

1. Triangle Counting - Gather [20 points]
Here, we will use MPI_Gather() to get local_counts from all the processes.

Create a buffer of size P on the root process and use this buffer to gather information from all processes (use MPI_Gather()).
Sum up the entries of the buffer to get the final triangle count.
Implement this as --strategy 1 in your triangle_counting_parallel executable.

2. Triangle Counting - Reduce [15 points]
Instead of creating a buffer and adding local_counts one by one, we can use the MPI_Reduce() API to directly obtain the aggregated result. Since we are summing up the local_count of the processes, use MPI_SUM as the reduction operator.

Implement this as --strategy 2 in your triangle_counting_parallel executable.

Output Format for Questions 1-2:

Your solution should be named triangle_counting_parallel.cpp and your Makefile should produce triangle_counting_parallel binary. Command line parameters to be supported:
--inputFile: The input graph file path as mentioned in the general instructions (datatype: string).
--strategy: The strategy to be used (datatype: integer). The valid values for --strategy are:
1 for Gather
2 for Reduce
Your parallel solution must output the following information:
World size (i.e., number of processes) (only root process).
Communication strategy used (only root process).
For each process,
Process rank (rank is between [0, P)).
Number of edges processed.
Number of triangles counted (local_count in the pseudocode).
Time taken for communication (in seconds). This should include the time taken to sum up the values (if any). Refer the pseudocode.
Total number of triangles in the graph (only root process).
Total number of unique triangles in the graph (only root process).
The total time taken for the entire execution. This should include the communication time and decomposition time (only root process).
Please note that the output format should strictly match the expected format (including "spaces" and "commas"). You can test your code using the test script as shown below. Note that you can run the python script only with slurm. Remember to invoke the script without srun.

$ python /scratch/assignment5/test_scripts/triangle_counting_tester.pyc --execPath=<absolute path of triangle_counting_parallel>
Sample output

World size : 4
Communication strategy : 1
1, 1383303, 125397, 0.000026
rank, edges, triangle_count, communication_time
0, 1383303, 161925, 0.007404
Number of triangles : 597206
Number of unique triangles : 199068
3, 1383305, 154830, 0.000005
Time taken (in seconds) : 0.08227
2, 1383303, 155054, 0.000005
PageRank using MPI
Similar to Triangle Counting, we will implement PageRank with MPI. Here, the work is distributed among P processes. For simplicity, every process will read the entire input graph and create its own graph data structure. For questions 3 and 4, we will use edge decomposition strategy to distribute the work among the processes (as done in Assignment 2). Refer to the pseudocode in the Triangle Counting using MPI section for the decomposition strategy.

The PageRank pseudocode for questions 3 and 4 is given below:

for each process P in parallel {
    communication_time = 0.0;
    for(i=0; i<max_iterations; i++) {
        for each vertex 'u' allocated to P {  // Loop 1
            edges_processed += outDegree(u)  // used in output validation
            for vertex 'v' in outNeighbor(u){
                next_page_rank[v] += (current_page_rank[u]/outdegree[u])
            }
        }

        // --- synchronization phase 1 start ---
        timer1.start();
        for each vertex 'u' allocated to P, aggregate (i.e., sum up) the value of next_page_rank[u] from all processes
        communication_time += timer1.stop();
        // --- synchronization phase 1 end -----

        for each vertex 'v' allocated to P {  // Loop 2
            new_value = PAGERANK(next_page_rank[v])
            current_page_rank[v] = new_value
        }
        Reset next_page_rank[v] to 0 for all vertices
    }
}
local_sum = 0
for each vertex 'v' allocated to P {  // Loop 3
    local_sum += current_page_rank[v]
}
// --- synchronization phase 2 start ---
if(P is root process){
    global_sum = Aggregated value of local_sum from all the processes using MPI_Reduce
    // print process statistics and other results
}
else{
    // print process statistics.
}
// --- synchronization phase 2 end ---

Key things to note:

For Loop 1 and Loop 2, each process only works on the vertices (and corresponding out edges) allocated to it.
You need to reset next_page_rank for all vertices at the end of every iteration.
The synchronization phase 1 is performed by all processes. We will use different strategies for this synchronization phase.
You should not perform synchronization individually for each vertex. Since each process works on a contiguous subset of vertices, you should synchronize the entire subset of vertices.
The global_sum computed in synchronization phase 2 is used for output verification. You should use MPI_Reduce() to sum up the local_sum of all processes. Note that this is a change from Assignment 4 where we used point-to-point communication to transfer this data.
Only the time spent on synchronization phase 1 is used for calculating communication time.
3. PageRank - Reduce and Scatter [25 points]
Here, we will use MPI_Reduce() and MPI_Scatterv() for communication. In this strategy,

The root process sums up the next_page_rank value for every vertex in the graph using MPI_Reduce().
Then, the root process sends the aggregated next_page_rank value of each vertex to its appropriate process. For example, if vertex v is allocated to process P1, the aggregated next_page_rank[v] is sent only to P1.
Implement this as --strategy 1 in your page_rank_parallel executable.

4. PageRank - Direct Reduce [25 points]
In the previous strategy, we summed up the next_page_rank value for each vertex on the root process and sent this aggregated value to the appropriate process. Instead, in this strategy,

Each process sums up the next_page_rank value, for every vertex allocated to that process, using MPI_Reduce().
Implement this as --strategy 2 in your page_rank_parallel executable.

Output Format for Questions 3-4:

Your solution should be named page_rank_parallel.cpp and your Makefile should produce page_rank_parallel binary. Command line parameters to be supported:
--inputFile: The input graph file path as mentioned in the general instructions (datatype: string).
--nIterations: The number of iterations to run PageRank (datatype: integer).
--strategy: The strategy to be used (datatype: integer). The valid values for --strategy are:
1 for Reduce and Scatter.
2 for Direct Reduce.
Your parallel solution must output the following information:
World size (i.e., number of processes) (only root process).
Communication strategy used. (only root process).
For each process,
Process rank (your rank is between [0, P)).
Number of edges processed (across all iterations) - This is the number of edges processed only in the first for loop across all iterations. Refer the pseudocode of PageRank.
Cumulative time taken for communication (in seconds). This should include the time taken to sum up the values (if any). Refer the pseudocode.
The sum of pageranks of all vertices (only root process).
The total time taken for the entire execution. This should include the communication time (only root process).
We will be using the integer version of pagerank for evaluating correctness. To run the integer version of PageRank, use the flag USE_INT=1 during make as follows:
$ make USE_INT=1 page_rank_parallel
Please note that the output format should strictly match the expected format (including "spaces" and "commas"). You can test your code using the test script as shown below. Note that you can run the python script only with slurm. Remember to invoke the script without srun.

$ python /scratch/assignment5/test_scripts/page_rank_tester.pyc --execPath=<absolute path of page_rank_parallel>
Sample output:

Using FLOAT
World size : 4
Communication strategy : 2
Iterations : 20
rank, num_edges, communication_time
0, 27666060, 0.748060
1, 27666060, 0.749769
2, 27666060, 0.733355
3, 27666100, 0.738441
Sum of page rank : 1966528.125000
Time taken (in seconds) : 1.26269
Submission Guidelines
Make sure that your solutions folder has the following files and sub-folders. Let's say your solutions folder is called my_assignment5_solutions. It should contain:
core/ -- The folder containing all core files. It is already available in the assignment 5 package. Do not modify it or remove any files.
Makefile -- Makefile for the project. This is the same Makefile provided in the serial package. Do not modify it.
triangle_counting_parallel.cpp
page_rank_parallel.cpp
To create the submission file, follow the steps below:
Enter in your solutions folder, and remove all the object/temporary files.
$ cd my_assignment5_solutions/
$ make clean
Create the tar.gz file.
$ tar cvzf assignment5.tar.gz *
which creates a compressed tar ball that contains the contents of the folder.
Validate the tar ball using the submission_validator.pyc script.
$ python /scratch/assignment5/test_scripts/submission_validator.pyc --tarPath=assignment5.tar.gz
Submit via CourSys by the deadline posted there.
Copyright © 2023 Keval Vora. All rights reserved.
