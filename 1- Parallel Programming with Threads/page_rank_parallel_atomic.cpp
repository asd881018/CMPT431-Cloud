#include "core/graph.h"
#include "core/utils.h"
#include <iomanip>
#include <iostream>
#include <stdlib.h>

#include <thread>
#include <vector>
#include <numeric>

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

inline void pageRankCal(Graph &g, int max_iters, uintV start, uintV end, std::atomic<PageRankType> *pr_next, PageRankType *pr_curr, uint t, double &thread_time, CustomBarrier &barrier)
{
  timer thread_timer;
  thread_timer.start();
  PageRankType old_value;
  PageRankType new_value;

  for (int iter = 0; iter < max_iters; iter++)
  {
    for (uintV u = start; u <= end; u++)
    {
      uintE out_degree = g.vertices_[u].getOutDegree();

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
    barrier.wait();
    for (uintV v = start; v <= end; v++)
    {
      PageRankType new_value = PAGE_RANK(pr_next[v].load());
      pr_curr[v] = new_value;
      pr_next[v].store(static_cast<PageRankType>(0));
    }
    barrier.wait();
  }
  // thread_time.thread_id = t;
  thread_time = thread_timer.stop();
}

void pageRankParallel(Graph &g, int max_iters, const uint n_workers)
{
  // Initialization
  uintV n = g.n_;
  PageRankType *pr_curr = new PageRankType[n];
  std::atomic<PageRankType> *pr_next = new std::atomic<PageRankType>[n];
  std::vector<uint> start(n_workers);
  std::vector<uint> end(n_workers);
  std::vector<std::thread> threads(n_workers);
  std::vector<double> thread_time_taken(n_workers);
  // std::vector<thread_time> thread_times(n_workers);
  std::vector<double> thread_times(n_workers);
  CustomBarrier barrier(n_workers);
  const uint thread_min_vertice = n / n_workers;
  uint remander_vertice = n % n_workers;
  uint curr_vertex = 0;

  for (uintV i = 0; i < n; i++)
  {
    pr_curr[i] = INIT_PAGE_RANK;
    pr_next[i] = 0.0;
  }

  timer t1;
  double time_taken = 0.0;

  // Create threads and distribute the work across T threads
  // -------------------------------------------------------------------

  t1.start();

  // Initialize start and end vertices for each thread
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

  for (uint t = 0; t < n_workers; t++)
  {
    threads[t] = std::thread(pageRankCal, std::ref(g), max_iters, start[t], end[t], pr_next, pr_curr, t, std::ref(thread_times[t]), std::ref(barrier));
  }

  for (int t = 0; t < n_workers; t++)
  {
    threads[t].join();
  }

  time_taken = t1.stop();

  // -------------------------------------------------------------------
  std::cout << "thread_id, time_taken\n";
  for (uint i = 0; i < n_workers; i++)
  {
    std::cout << i << ", " << thread_times[i] << "\n";
  }
  // Print the above statistics for each thread
  // Example output for 2 threads:
  // thread_id, time_taken
  // 0, 0.12
  // 1, 0.12

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
          {"inputFile", "Input graph file path",
           cxxopts::value<std::string>()->default_value(
               "/scratch/input_graphs/roadNet-CA")},
      });

  auto cl_options = options.parse(argc, argv);
  uint n_workers = cl_options["nWorkers"].as<uint>();
  uint max_iterations = cl_options["nIterations"].as<uint>();
  std::string input_file_path = cl_options["inputFile"].as<std::string>();

#ifdef USE_INT
  std::cout << "Using INT\n";
#else
  std::cout << "Using FLOAT\n";
#endif
  std::cout << std::fixed;
  std::cout << "Number of workers : " << n_workers << "\n";

  Graph g;
  std::cout << "Reading graph\n";
  g.readGraphFromBinary<int>(input_file_path);
  std::cout << "Created graph\n";
  // pageRankSerial(g, max_iterations);
  pageRankParallel(g, max_iterations, n_workers);
  // pageRankParallel2(&g, max_iterations, n_workers);

  return 0;
}
