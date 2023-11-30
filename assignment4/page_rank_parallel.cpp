#include <iostream>
#include <cstdio>
#include "core/utils.h"
#include "core/graph.h"

#include <numeric>
#include <mpi.h>
#include <stdio.h>

#ifdef USE_INT
#define INIT_PAGE_RANK 100000
#define EPSILON 1000
#define PAGE_RANK(x) (15000 + (5 * x) / 6)
#define CHANGE_IN_PAGE_RANK(x, y) std::abs(x - y)
#define PAGERANK_MPI_TYPE MPI_LONG
#define PR_FMT "%ld"
typedef int64_t PageRankType;
#else
#define INIT_PAGE_RANK 1.0
#define EPSILON 0.01
#define DAMPING 0.85
#define PAGE_RANK(x) (1 - DAMPING + DAMPING * x)
#define CHANGE_IN_PAGE_RANK(x, y) std::fabs(x - y)
#define PAGERANK_MPI_TYPE MPI_FLOAT
#define PR_FMT "%f"
typedef float PageRankType;
#endif

const int ROOT_PROCESS = 0;

void pageRankSerial(Graph &g, int max_iters)
{
    uintV n = g.n_;
    double time_taken;
    timer t1;
    PageRankType *pr_curr = new PageRankType[n];
    PageRankType *pr_next = new PageRankType[n];

    t1.start();
    for (uintV i = 0; i < n; i++)
    {
        pr_curr[i] = INIT_PAGE_RANK;
        pr_next[i] = 0.0;
    }

    // Push based pagerank
    // -------------------------------------------------------------------
    for (int iter = 0; iter < max_iters; iter++)
    {
        // for each vertex 'u', process all its outNeighbors 'v'
        for (uintV u = 0; u < n; u++)
        {
            uintE out_degree = g.vertices_[u].getOutDegree();
            for (uintE i = 0; i < out_degree; i++)
            {
                uintV v = g.vertices_[u].getOutNeighbor(i);
                pr_next[v] += (pr_curr[u] / out_degree);
            }
        }
        for (uintV v = 0; v < n; v++)
        {
            pr_next[v] = PAGE_RANK(pr_next[v]);

            // reset pr_curr for the next iteration
            pr_curr[v] = pr_next[v];
            pr_next[v] = 0.0;
        }
    }
    // -------------------------------------------------------------------

    // For every thread, print the following statistics:
    // rank, num_edges, communication_time
    // 0, 344968860, 1.297778
    // 1, 344968860, 1.247763
    // 2, 344968860, 0.956243
    // 3, 344968880, 0.467028

    PageRankType sum_of_page_ranks = 0;
    for (uintV u = 0; u < n; u++)
    {
        sum_of_page_ranks += pr_curr[u];
    }
    time_taken = t1.stop();
    std::printf("Sum of page rank : " PR_FMT "\n", sum_of_page_ranks);
    std::printf("Time taken (in seconds) : %f\n", time_taken);
    delete[] pr_curr;
    delete[] pr_next;
}

void assignWorkBasedOnEdges(Graph &g, uintV *partitionArray, uintV n, const uint processes, uint)
{
    uintE edgeSum = 0, edges_per_process = 0, edgeCount = 0;
    int curr_process = 0;
    partitionArray[processes] = n;

    edgeSum = std::accumulate(g.vertices_, g.vertices_ + n, 0, [](uintE sum, Vertex &v)
                              { return sum + v.getOutDegree(); });
    edges_per_process = edgeSum / processes;

    uintV u = 0;
    while (u < n)
    {
        edgeCount += g.vertices_[u].getOutDegree();

        if (edgeCount > edges_per_process * curr_process)
        {
            partitionArray[curr_process] = u;
            curr_process++;
            if (curr_process == processes)
                break;
        }
        u++;
    }
}

void pageRankParallelStrategyOne(Graph &g, int maxIters, uintV *partitionArray)
{
    uintV n = g.n_;
    long edges_processed = 0;
    timer communication_timer;
    double communication_time = 0.0;

    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    PageRankType *pr_curr = new PageRankType[n];
    PageRankType *pr_next = new PageRankType[n];
    PageRankType *pr_recv = new PageRankType[n];

    for (uintV i = 0; i < n; i++)
    {
        pr_curr[i] = INIT_PAGE_RANK;
        pr_next[i] = 0.0;
        pr_recv[i] = 0.0;
    }

    for (int i = 0; i < maxIters; i++)
    {
        // Loop 1
        for (uintV u = partitionArray[world_rank]; u < partitionArray[world_rank + 1]; u++)
        {
            uintE out_degree = g.vertices_[u].getOutDegree();
            edges_processed += out_degree;
            for (uintE i = 0; i < out_degree; i++)
            {
                uintV v = g.vertices_[u].getOutNeighbor(i);
                pr_next[v] += (pr_curr[u] / out_degree);
            }
        }

        // --- synchronization phase 1 start ---
        communication_timer.start();
        if (world_rank == ROOT_PROCESS)
        {
            for (int i = 1; i < world_size; i++)
            {
                MPI_Recv(pr_recv, n, PAGERANK_MPI_TYPE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                std::for_each(pr_next, pr_next + n, [pr_recv, idx = 0](PageRankType &pr) mutable
                              { pr += pr_recv[idx++]; });
            }
            for (int i = 1; i < world_size; i++)
            {
                MPI_Send(&pr_next[partitionArray[i]], partitionArray[i + 1] - partitionArray[i], PAGERANK_MPI_TYPE, i, 0, MPI_COMM_WORLD);
            }
        }
        else
        {
            MPI_Send(pr_next, n, PAGERANK_MPI_TYPE, ROOT_PROCESS, 0, MPI_COMM_WORLD);
            MPI_Recv(pr_recv, partitionArray[world_rank + 1] - partitionArray[world_rank], PAGERANK_MPI_TYPE, ROOT_PROCESS, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        communication_time += communication_timer.stop();
        // --- synchronization phase 1 end -----
        // Loop 2
        for (uintV v = partitionArray[world_rank]; v < partitionArray[world_rank + 1]; v++)
        {
            uintV v_new = v - partitionArray[world_rank];

            if (world_rank == ROOT_PROCESS)
            {
                pr_next[v] = PAGE_RANK(pr_next[v_new]);
            }
            else
            {
                pr_next[v] = PAGE_RANK(pr_recv[v_new]);
            }
            pr_curr[v] = pr_next[v];
        }

        // Reset next_page_rank[v] to 0 for all vertices
        for (uintV v = 0; v < n; v++)
        {
            pr_next[v] = 0.0;
        }
    }

    // Loop 3
    PageRankType local_sum = std::accumulate(pr_curr + partitionArray[world_rank], pr_curr + partitionArray[world_rank + 1], 0.0);

    // --- synchronization phase 2 start -----
    PageRankType global_sum = 0, sum_recv = 0;
    if (world_rank == ROOT_PROCESS)
    {
        global_sum += local_sum;
        for (int i = 1; i < world_size; i++)
        {
            MPI_Recv(&sum_recv, 1, PAGERANK_MPI_TYPE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            global_sum += sum_recv;
        }
    }
    else
    {
        MPI_Send(&local_sum, 1, PAGERANK_MPI_TYPE, ROOT_PROCESS, 0, MPI_COMM_WORLD);
    }
    // --- synchronization phase 2 end -----

    std::printf("%d, %ld, %f\n", world_rank, edges_processed, communication_time);

    if (world_rank == ROOT_PROCESS)
    {
        std::printf("Sum of page rank : " PR_FMT "\n", global_sum);
    }

    delete[] pr_curr;
    delete[] pr_next;
    delete[] pr_recv;
}

int main(int argc, char *argv[])
{
    cxxopts::Options options("page_rank_push", "Calculate page_rank using serial and parallel execution");
    options.add_options("", {
                                {"nIterations", "Maximum number of iterations", cxxopts::value<uint>()->default_value(DEFAULT_MAX_ITER)},
                                {"strategy", "Strategy to be used", cxxopts::value<uint>()->default_value(DEFAULT_STRATEGY)},
                                {"inputFile", "Input graph file path", cxxopts::value<std::string>()->default_value("/scratch/input_graphs/roadNet-CA")},
                            });

    auto cl_options = options.parse(argc, argv);
    uint strategy = cl_options["strategy"].as<uint>();
    uint max_iterations = cl_options["nIterations"].as<uint>();
    std::string input_file_path = cl_options["inputFile"].as<std::string>();

    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);


    if (world_rank == 0)
    {
#ifdef USE_INT
        std::printf("Using INT\n");
#else
        std::printf("Using FLOAT\n");
#endif
        // Get the world size and print it out here
        std::printf("World size : %d\n", world_size);
        std::printf("Communication strategy : %d\n", strategy);
        std::printf("Iterations : %d\n", max_iterations);
        std::printf("rank, num_edges, communication_time\n");
    }

    Graph g;
    g.readGraphFromBinary<int>(input_file_path);

    uintV n = g.n_;
    uintV *partitionArray = new uintV[world_size];
    assignWorkBasedOnEdges(g, partitionArray, n, world_size, world_rank);

    timer total_timer;
    total_timer.start();

    switch (strategy)
    {
    case 0:
        pageRankSerial(g, max_iterations);
        break;
    case 1:
        pageRankParallelStrategyOne(g, max_iterations, partitionArray);
        break;
    }

    if (world_rank == 0)
    {
        double total_time = total_timer.stop();
        std::printf("Time taken (in seconds) : %f\n", total_time);
    }

    MPI_Finalize();
    delete[] partitionArray;
    return 0;
}