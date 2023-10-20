#include "core/graph.h"
#include "core/utils.h"
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <thread>

#ifdef USE_INT
#define INIT_PAGE_RANK 100000
#define EPSILON 1000
#define PAGE_RANK(x) (15000 + (5 * x) / 6)
#define CHANGE_IN_PAGE_RANK(x, y) std::abs(x - y)
typedef int64_t PageRankType;
#else
#define INIT_PAGE_RANK 1.0
#define EPSILON 0.01
#define DAMPING 0.85
#define PAGE_RANK(x) (1 - DAMPING + DAMPING * x)
#define CHANGE_IN_PAGE_RANK(x, y) std::fabs(x - y)
typedef float PageRankType;
#endif

// thread_id, num_vertices, num_edges, barrier1_time, barrier2_time, getNextVertex_time, total_time
struct thread_status
{
  uint thread_id;
  long num_vertices;
  long num_edges;
  double barrier1_time;
  double barrier2_time;
  double getNextVertex_time;
  double total_time;
};

void printPageRankCountStatistics(const std::vector<thread_status> &thread_status, int n_workers, PageRankType sum_of_page_ranks, double partition_time, double time_taken)
{
  // Print the above statistics for each thread
  // Example output for 4 threads:
  // thread_id, num_vertices, num_edges, barrier1_time, barrier2_time, getNextVertex_time, total_time
  // 0, 9797860, 27666060, 0.013256, 0.006471, 0, 0.660568
  // 1, 9760160, 27666080, 0.019151, 0.012438, 0, 0.659655
  // 2, 9893740, 27666080, 0.012808, 0.009866, 0, 0.658618
  // 3, 9973860, 27666060, 0.021313, 0.007165, 0, 0.657224
  // Sum of page rank : 1966528.125000
  // Partitioning time (in seconds) : 0.000000
  // Time taken (in seconds) : 0.661586

  std::cout << "thread_id, num_vertices, num_edges, barrier1_time, barrier2_time, getNextVertex_time, total_time\n";
  for (uint i = 0; i < n_workers; i++)
  {
    std::cout << i << ", " << thread_status[i].num_vertices << ", " << thread_status[i].num_edges << ", " << thread_status[i].barrier1_time << ", " << thread_status[i].barrier2_time << ", " << thread_status[i].getNextVertex_time << ", " << thread_status[i].total_time << "\n";
  }

  std::cout << "Sum of page rank : " << sum_of_page_ranks << "\n";
  std::cout << "Partitioning time (in seconds) : " << partition_time << "\n";
  std::cout << "Time taken (in seconds) : " << time_taken << "\n";
}

inline void pageRankCal(Graph &g, int max_iters, uintV start, uintV end, std::atomic<PageRankType> *pr_next, PageRankType *pr_curr, uint t, thread_status &thread_status, CustomBarrier &barrier)
{
  timer thread_timer, barrier1_timer, barrier2_timer;
  thread_status.getNextVertex_time = 0;
  thread_timer.start();
  PageRankType old_value;
  PageRankType new_value;

  for (int iter = 0; iter < max_iters; iter++)
  {
    for (uintV u = start; u <= end; u++)
    {
      uintE out_degree = g.vertices_[u].getOutDegree();
      thread_status.num_vertices = end - start + 1;
      thread_status.num_edges += out_degree;

      for (uintE i = 0; i < out_degree; i++)
      {
        uintV v = g.vertices_[u].getOutNeighbor(i);
        PageRankType delta = pr_curr[u] / (PageRankType)out_degree;
        // Use atomic compare_exchange to update pr_next[v]
        old_value = pr_next[v].load(std::memory_order_relaxed);
        new_value = old_value + delta;

        // Use compare_exchange_weak to atomically update pr_next[v]
        // https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
        while (!pr_next[v].compare_exchange_weak(old_value, new_value, std::memory_order_relaxed))
        {
          // Retry until successful atomic update
          new_value = old_value + delta;
        }
      }
    }
    barrier1_timer.start();
    barrier.wait();
    thread_status.barrier1_time = barrier1_timer.stop();

    for (uintV v = start; v <= end; v++)
    {
      PageRankType new_value = PAGE_RANK(pr_next[v].load());
      pr_curr[v] = new_value;
      pr_next[v].store(static_cast<PageRankType>(0));
    }
    barrier2_timer.start();
    barrier.wait();
    thread_status.barrier2_time = barrier2_timer.stop();
  }
  thread_status.total_time = thread_timer.stop();
}

void pageRankSerial(Graph &g, int max_iters)
{
  uintV n = g.n_;

  PageRankType *pr_curr = new PageRankType[n];
  PageRankType *pr_next = new PageRankType[n];

  for (uintV i = 0; i < n; i++)
  {
    pr_curr[i] = INIT_PAGE_RANK;
    pr_next[i] = 0.0;
  }

  // Push based pagerank
  timer t1;
  double time_taken = 0.0;
  // Create threads and distribute the work across T threads
  // -------------------------------------------------------------------
  t1.start();
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
  time_taken = t1.stop();
  // -------------------------------------------------------------------

  PageRankType sum_of_page_ranks = 0;
  for (uintV u = 0; u < n; u++)
  {
    sum_of_page_ranks += pr_curr[u];
  }
  std::cout << "Sum of page rank : " << sum_of_page_ranks << "\n";
  std::cout << "Time taken (in seconds) : " << time_taken << "\n";
  delete[] pr_curr;
  delete[] pr_next;
}

void pageRankParallelStrategyOne(Graph &g, int max_iters, const uint n_workers)
{
  // Initialization
  uintV n = g.n_;
  PageRankType *pr_curr = new PageRankType[n];
  std::atomic<PageRankType> *pr_next = new std::atomic<PageRankType>[n];
  std::vector<uint> start(n_workers);
  std::vector<uint> end(n_workers);
  std::vector<std::thread> threads(n_workers);
  std::vector<thread_status> thread_status(n_workers);
  CustomBarrier barrier(n_workers);
  const uint thread_min_vertice = n / n_workers;
  uint remander_vertice = n % n_workers;
  uint curr_vertex = 0;

  for (uintV i = 0; i < n; i++)
  {
    pr_curr[i] = INIT_PAGE_RANK;
    pr_next[i] = 0.0;
  }

  timer t1, partition_timer;
  double time_taken = 0.0;
  double partition_time = 0.0;

  // Create threads and distribute the work across T threads
  // -------------------------------------------------------------------

  t1.start();

  // Initialize start and end vertices for each thread
  partition_timer.start();
  for (uint i = 0; i < n_workers; i++)
  {
    start[i] = curr_vertex;
    if (remander_vertice > 0)
    {
      end[i] = curr_vertex + thread_min_vertice;
      remander_vertice--;
    }
    else
    {
      end[i] = curr_vertex + thread_min_vertice - 1;
    }
    curr_vertex = end[i] + 1;
  }
  partition_time = partition_timer.stop();

  for (uint t = 0; t < n_workers; t++)
  {
    threads[t] = std::thread(pageRankCal, std::ref(g), max_iters, start[t], end[t], pr_next, pr_curr, t, std::ref(thread_status[t]), std::ref(barrier));
  }

  for (int t = 0; t < n_workers; t++)
  {
    threads[t].join();
  }

  time_taken = t1.stop();

  // -------------------------------------------------------------------
  // std::cout << "thread_id, time_taken\n";
  // for (uint i = 0; i < n_workers; i++)
  // {
  //   std::cout << i << ", " << thread_times[i] << "\n";
  // }

  PageRankType sum_of_page_ranks = 0;
  for (uintV u = 0; u < n; u++)
  {
    sum_of_page_ranks += pr_curr[u];
  }

  printPageRankCountStatistics(thread_status, n_workers, sum_of_page_ranks, partition_time, time_taken);

  delete[] pr_curr;
  delete[] pr_next;
}

int main(int argc, char *argv[])
{
  cxxopts::Options options(
      "page_rank_push",
      "Calculate page_rank using serial and parallel execution");
  options.add_options(
      "",
      {
          {"nWorkers", "Number of workers",
           cxxopts::value<uint>()->default_value(DEFAULT_NUMBER_OF_WORKERS)},
          {"nIterations", "Maximum number of iterations",
           cxxopts::value<uint>()->default_value(DEFAULT_MAX_ITER)},
          {"strategy", "Strategy to be used",
           cxxopts::value<uint>()->default_value(DEFAULT_STRATEGY)},
          {"granularity", "Granularity to be used",
           cxxopts::value<uint>()->default_value(DEFAULT_GRANULARITY)},
          {"inputFile", "Input graph file path",
           cxxopts::value<std::string>()->default_value(
               "/scratch/input_graphs/roadNet-CA")},
      });

  auto cl_options = options.parse(argc, argv);
  uint n_workers = cl_options["nWorkers"].as<uint>();
  uint strategy = cl_options["strategy"].as<uint>();
  uint max_iterations = cl_options["nIterations"].as<uint>();
  uint granularity = cl_options["granularity"].as<uint>();
  std::string input_file_path = cl_options["inputFile"].as<std::string>();

#ifdef USE_INT
  std::cout << "Using INT\n";
#else
  std::cout << "Using FLOAT\n";
#endif
  std::cout << std::fixed;
  std::cout << "Number of workers : " << n_workers << "\n";
  std::cout << "Task decomposition strategy : " << strategy << "\n";
  std::cout << "Iterations : " << max_iterations << "\n";
  std::cout << "Iterations : " << max_iterations << "\n";
  std::cout << "Granularity : " << granularity << "\n";

  Graph g;
  std::cout << "Reading graph\n";
  g.readGraphFromBinary<int>(input_file_path);
  std::cout << "Created graph\n";
  switch (strategy)
  {
  case 0:
    std::cout << "\nSerial\n";
    pageRankSerial(g, max_iterations);
    break;
  case 1:
    std::cout << "\nVertex-based work partitioning\n";
    pageRankParallelStrategyOne(g, max_iterations, n_workers);
    break;
  case 2:
    std::cout << "\nEdge-based work partitioning\n";
    break;
  case 3:
    std::cout << "\nDynamic task mapping\n";
    break;
  default:
    break;
  }

  return 0;
}
