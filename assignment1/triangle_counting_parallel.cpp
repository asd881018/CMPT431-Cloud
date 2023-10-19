#include "core/graph.h"
#include "core/utils.h"
#include <future>
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <thread>
#include <vector>
#include <numeric>

long countTriangles(uintV *array1, uintE len1, uintV *array2, uintE len2,
                    uintV u, uintV v)
{

  uintE i = 0, j = 0; // indexes for array1 and array2
  long count = 0;

  if (u == v)
    return count;

  while ((i < len1) && (j < len2))
  {
    if (array1[i] == array2[j])
    {
      if ((array1[i] != u) && (array1[i] != v))
      {
        count++;
      }
      i++;
      j++;
    }
    else if (array1[i] < array2[j])
    {
      i++;
    }
    else
    {
      j++;
    }
  }
  return count;
}

struct thread_status
{
  uint thread_id;
  long triangle_count;
  double time_taken;
};
void triangleCountSerial(Graph &g, uint n_workers)
{
    uintV n = g.n_;
    long triangle_count = 0;
    double time_taken = 0.0;
    timer t1;

    // The outNghs and inNghs for a given vertex are already sorted

    // Create threads and distribute the work across T threads
    // -------------------------------------------------------------------
    t1.start();
    // Process each edge <u,v>
    for (uintV u = 0; u < n; u++)
    {
        // For each outNeighbor v, find the intersection of inNeighbor(u) and
        // outNeighbor(v)
        uintE out_degree = g.vertices_[u].getOutDegree();

        for (uintE i = 0; i < out_degree; i++)
        {
            uintV v = g.vertices_[u].getOutNeighbor(i);
            triangle_count += countTriangles(g.vertices_[u].getInNeighbors(),
                                             g.vertices_[u].getInDegree(),
                                             g.vertices_[v].getOutNeighbors(),
                                             g.vertices_[v].getOutDegree(), u, v);
        }
    }
    time_taken = t1.stop();
    // -------------------------------------------------------------------
    // Here, you can just print the number of non-unique triangles counted by each
    // thread std::cout << "thread_id, triangle_count, time_taken\n"; Print the
    // above statistics for each thread Example output for 2 threads: thread_id,
    // triangle_count, time_taken 1, 102, 0.12 0, 100, 0.12

    // Print the overall statistics
    std::cout << "Number of triangles : " << triangle_count << "\n";
    std::cout << "Number of unique triangles : " << triangle_count / 3 << "\n";
    std::cout << "Time taken (in seconds) : " << std::setprecision(TIME_PRECISION)
              << time_taken << "\n";
}
void triangleParallel(Graph &g, const uint &n_workers)
{
  uint n = g.n_;
  long triangle_count = 0;
  double time_taken = 0.0;
  timer t1;

  timer thread_timer;
  const int thread_n = n / n_workers;
  int remander = n % n_workers;

  // The outNghs and inNghs for a given vertex are already sorted

  // Create threads and distribute the work across T threads
  // -------------------------------------------------------------------
  std::vector<std::thread> threads(n_workers);
  std::vector<thread_status> thread_status(n_workers);

  t1.start();
  for (uint t = 0; t < n_workers; t++)
  {
    thread_timer.start();
    threads[t] = std::thread([&g, t, thread_n, remander, &thread_status, &n_workers]() {
      uintV n = g.n_;
      long triangle_count = 0;
      double time_taken = 0.0;
      timer t1;
      uintV start = t * thread_n;
      uintV end = (t + 1) * thread_n;
      if (t == n_workers - 1)
      {
        end += remander;
      }
      // Process each edge <u,v>
      for (uintV u = start; u < end; u++)
      {
        // For each outNeighbor v, find the intersection of inNeighbor(u) and
        // outNeighbor(v)
        uintE out_degree = g.vertices_[u].getOutDegree();

        for (uintE i = 0; i < out_degree; i++)
        {
          uintV v = g.vertices_[u].getOutNeighbor(i);
          triangle_count += countTriangles(g.vertices_[u].getInNeighbors(),
                                           g.vertices_[u].getInDegree(),
                                           g.vertices_[v].getOutNeighbors(),
                                           g.vertices_[v].getOutDegree(), u, v);
        }
      }
      time_taken = t1.stop();
      thread_status[t] = {t, triangle_count, time_taken};
    });
  }
  for (uint t = 0; t < n_workers; t++)
  {
    threads[t].join();
  }
  time_taken = t1.stop();
  // -------------------------------------------------------------------
  // Here, you can just print the number of non-unique triangles counted by each
  // thread std::cout << "thread_id, triangle_count, time_taken\n"; Print the
  // above statistics for each thread Example output for 2 threads: thread_id,
  // triangle_count, time_taken 1, 102, 0.12 0, 100, 0.12
  std::cout << "thread_id, triangle_count, time_taken\n";
  for (uint t = 0; t < n_workers; t++)
  {
    std::cout << thread_status[t].thread_id << ", " << thread_status[t].triangle_count << ", " << thread_status[t].time_taken << "\n";
    triangle_count += thread_status[t].triangle_count;
  }

  // Print the overall statistics
  std::cout << "Number of triangles : " << triangle_count << "\n";
  std::cout << "Number of unique triangles : " << triangle_count / 3 << "\n";
  std::cout << "Time taken (in seconds) : " << std::setprecision(TIME_PRECISION)
            << time_taken << "\n";
}


int main(int argc, char *argv[])
{
  cxxopts::Options options(
      "triangle_counting_serial",
      "Count the number of triangles using serial and parallel execution");
  options.add_options(
      "custom",
      {
          {"nWorkers", "Number of workers",
           cxxopts::value<uint>()->default_value(DEFAULT_NUMBER_OF_WORKERS)},
          {"inputFile", "Input graph file path",
           cxxopts::value<std::string>()->default_value(
               "/scratch/input_graphs/roadNet-CA")},
      });

  auto cl_options = options.parse(argc, argv);
  uint n_workers = cl_options["nWorkers"].as<uint>();
  std::string input_file_path = cl_options["inputFile"].as<std::string>();
  std::cout << std::fixed;
  std::cout << "Number of workers : " << n_workers << "\n";

  Graph g;
  std::cout << "Reading graph\n";
  g.readGraphFromBinary<int>(input_file_path);
  std::cout << "Created graph\n";

  // triangleCountSerial(g, n_workers);
  triangleParallel(g, n_workers);

  return 0;
}
