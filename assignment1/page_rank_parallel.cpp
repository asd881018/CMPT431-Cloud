#include "core/graph.h"
#include "core/utils.h"
#include <iomanip>
#include <iostream>
#include <stdlib.h>

#include <thread>
#include <vector>

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

/* Slides:
#defein CURR i % 2
#define NEXT (i + 1) % 2

create T threads
for each thread in parallel {
  for (i=0; i<k; ++i){
    for v in subset(V, tid){
      sum = 0;
      for u in inNeighbors(v){
        sum += pr[CURR][u] / outDegree(u);
      }
      pr[NEXT][v] = PAGE_RANK(sum);
    }
    barrier();
  }
}
*/

struct thread_status
{
  uint thread_id;
  double time_taken;
};

void pageRankParallel(Graph &g, int max_iters, uint n_workers) {
  uintV n = g.n_;

  // Initialization
  PageRankType *pr_curr = new PageRankType[n];
  PageRankType *pr_next = new PageRankType[n];

  for (uintV i = 0; i < n; i++) {
    pr_curr[i] = INIT_PAGE_RANK;
    pr_next[i] = 0.0;
  }

  // Push based pagerank
  timer t1;
  // timer thread_timer;
  double time_taken = 0.0;
  // Create threads and distribute the work across T threads
  // -------------------------------------------------------------------
  std::vector<std::thread> threads(n_workers);
  std::vector<double> thread_time_taken(n_workers);
  std::vector<thread_status> thread_status(n_workers);
  t1.start();
  for (int iter = 0; iter < max_iters; iter++) {
    for (uint i = 0; i < n_workers; i++) {
      threads[i] = std::thread([&, i]() {
        timer thread_timer;
        thread_timer.start();
        for (uintV u = i; u < n; u += n_workers) {
          uintE out_degree = g.vertices_[u].getOutDegree();
          for (uintE i = 0; i < out_degree; i++) {
            uintV v = g.vertices_[u].getOutNeighbor(i);
            pr_next[v] += (pr_curr[u] / out_degree);
          }
        }
        thread_status[i].thread_id = i;
        thread_status[i].time_taken = thread_timer.stop();
      });
    }
    for (uint i = 0; i < n_workers; i++) {
      threads[i].join();
    }
    for (uintV v = 0; v < n; v++) {
      pr_next[v] = PAGE_RANK(pr_next[v]);

      // reset pr_curr for the next iteration
      pr_curr[v] = pr_next[v];
      pr_next[v] = 0.0;
    }
  }
  for (uint t = 0; t < n_workers; t++)
  {
    threads[t].join();
  }

  time_taken = t1.stop();
  

  // t1.start();
  // for (int iter = 0; iter < max_iters; iter++) {
  //   // for each vertex 'u', process all its outNeighbors 'v'
  //   for (uintV u = 0; u < n; u++) {
  //     uintE out_degree = g.vertices_[u].getOutDegree();
  //     for (uintE i = 0; i < out_degree; i++) {
  //       uintV v = g.vertices_[u].getOutNeighbor(i);
  //       pr_next[v] += (pr_curr[u] / out_degree);
  //     }
  //   }
  //   for (uintV v = 0; v < n; v++) {
  //     pr_next[v] = PAGE_RANK(pr_next[v]);

  //     // reset pr_curr for the next iteration
  //     pr_curr[v] = pr_next[v];
  //     pr_next[v] = 0.0;
  //   }
  // }
  // time_taken = t1.stop();
  // -------------------------------------------------------------------
  std::cout << "thread_id, time_taken\n";
  for (uint i = 0; i < n_workers; i++) {
    std::cout << thread_status[i].thread_id << ", " << thread_status[i].time_taken << "\n";
  }
  // Print the above statistics for each thread
  // Example output for 2 threads:
  // thread_id, time_taken
  // 0, 0.12
  // 1, 0.12

  PageRankType sum_of_page_ranks = 0;
  for (uintV u = 0; u < n; u++) {
    sum_of_page_ranks += pr_curr[u];
  }
  std::cout << "Sum of page rank : " << sum_of_page_ranks << "\n";
  std::cout << "Time taken (in seconds) : " << time_taken << "\n";
  delete[] pr_curr;
  delete[] pr_next;
}

int main(int argc, char *argv[]) {
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

  return 0;
}
