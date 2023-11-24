#include <iostream>
#include <cstdio>
#include "core/utils.h"
#include "core/graph.h"

#include <numeric>
#include <mpi.h>
#include <stdio.h>

const int ROOT_PROCESS = 0;

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
      else
      {
        // triangle with self-referential edge -> ignore
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

void triangleCountSerial(Graph &g)
{
  uintV n = g.n_;
  long triangle_count = 0;
  double time_taken;
  timer t1;
  t1.start();
  for (uintV u = 0; u < n; u++)
  {
    uintE out_degree = g.vertices_[u].getOutDegree();
    for (uintE i = 0; i < out_degree; i++)
    {
      uintV v = g.vertices_[u].getOutNeighbor(i);
      triangle_count += countTriangles(g.vertices_[u].getInNeighbors(),
                                       g.vertices_[u].getInDegree(),
                                       g.vertices_[v].getOutNeighbors(),
                                       g.vertices_[v].getOutDegree(),
                                       u,
                                       v);
    }
  }

  // For every thread, print out the following statistics:
  // rank, edges, triangle_count, communication_time
  // 0, 17248443, 144441858, 0.000074
  // 1, 17248443, 152103585, 0.000020
  // 2, 17248443, 225182666, 0.000034
  // 3, 17248444, 185596640, 0.000022

  time_taken = t1.stop();

  // Print out overall statistics
  std::printf("Number of triangles : %ld\n", triangle_count);
  std::printf("Number of unique triangles : %ld\n", triangle_count / 3);
  std::printf("Time taken (in seconds) : %f\n", time_taken);
}

void assignWorkBasedOnEdges(Graph &g, uintV *partitionArray, uintV n, uintE m, const uint processes, uint global_rank)
{
  uintE edgeSum = 0, edges_per_process = 0, edgeCount = 0;
  int curr_process = 0;
  partitionArray[processes] = n;

  edgeSum = std::accumulate(g.vertices_, g.vertices_ + n, 0, [](uintE sum, Vertex &v)
                            { return sum + v.getOutDegree(); });
  edges_per_process = edgeSum / processes;

  uintV u = 0;
  while(u < n)
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

  /* This method id too slow*/

  // uintE edgeSum = std::accumulate(g.vertices_, g.vertices_ + n, 0, [](uintE sum, Vertex &v) { return sum + v.getOutDegree(); });
  // uintE edges_per_process = edgeSum / processes; // m/P
  // uint start_vertex = 0, end_vertex = 0;
  // partitionArray[processes] = n;

  // for (int i = 0; i < processes; i++)
  // {
  //   start_vertex = end_vertex;
  //   uintE count = 0; // Count of edges for this process

  //   while (end_vertex < n)
  //   {
  //     // add vertices unil we reach m/P edges
  //     count += g.vertices_[end_vertex].getOutDegree();
  //     end_vertex++;

  //     // Check if this process has enough edges or if we've covered all vertices
  //     if (count >= edges_per_process)
  //     {
  //       partitionArray[i] = end_vertex;
  //       break;
  //     }
  //   }
  //   if (i == global_rank)
  //   {
  //     break;
  //   }
  // }
}

// triangleCountMPI(g, world_rank, partitionArray);
void triangleCountMPI(Graph &g, int world_rank, uintV *partitionArray)
{

  uintE edges_processed = 0;
  long local_count = 0, global_count = 0;
  timer communication_timer;

  int process_rank, process_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &process_size);

  // Each process will work on vertices [start_vertex, end_vertex).
  for (uintV u = partitionArray[process_rank]; u < partitionArray[process_rank + 1]; u++)
  {
    uintE out_degree = g.vertices_[u].getOutDegree();
    edges_processed += out_degree; // used in output validation

    // for vertex 'v' in outNeighbor(u)
    for (uintE i = 0; i < out_degree; i++)
    {
      uintV v = g.vertices_[u].getOutNeighbor(i);
      local_count += countTriangles(g.vertices_[u].getInNeighbors(),
                                    g.vertices_[u].getInDegree(),
                                    g.vertices_[v].getOutNeighbors(),
                                    g.vertices_[v].getOutDegree(),
                                    u,
                                    v);
    }
  }

  // --- synchronization phase start ---
  communication_timer.start();
  if (process_rank == ROOT_PROCESS)
  {
    global_count += local_count;
    for (int i = 1; i < process_size; i++)
    {
      long recv_count = 0;
      MPI_Recv(&recv_count, 1, MPI_LONG, i, ROOT_PROCESS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      global_count += recv_count;
    }
  }
  else
  {
    MPI_Send(&local_count, 1, MPI_LONG, ROOT_PROCESS, 0, MPI_COMM_WORLD);
  }
  double communication_time = communication_timer.stop();
  // --- synchronization phase end -----

  std::printf("%d, %d, %ld, %f\n", process_rank, edges_processed, local_count, communication_time);

  if (process_rank == ROOT_PROCESS)
  {
    std::printf("Number of triangles : %ld\n", global_count);
    std::printf("Number of unique triangles : %ld\n", global_count / 3);
  }
}

int main(int argc, char *argv[])
{
  cxxopts::Options options("triangle_counting_serial", "Count the number of triangles using serial and parallel execution");
  options.add_options("custom", {
                                    {"strategy", "Strategy to be used", cxxopts::value<uint>()->default_value(DEFAULT_STRATEGY)},
                                    {"inputFile", "Input graph file path", cxxopts::value<std::string>()->default_value("/scratch/input_graphs/roadNet-CA")},
                                });

  auto cl_options = options.parse(argc, argv);
  uint strategy = cl_options["strategy"].as<uint>();
  std::string input_file_path = cl_options["inputFile"].as<std::string>();

  // Initialize the MPI environment
  // MPI_Init(&argc, &argv);

  MPI_Init(NULL, NULL);

  int world_size, world_rank;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

  if (world_rank == ROOT_PROCESS)
  {
    // Get the world size and print it out here
    std::printf("World size : %d\n", world_size);
    std::printf("Communication strategy : %d\n", strategy);
    std::printf("rank, edges, triangle_count, communication_time\n");
  }

  Graph g;
  g.readGraphFromBinary<int>(input_file_path);

  uintV n = g.n_;
  uintE m = g.m_;
  uintV *partitionArray = new uintV[world_rank];
  assignWorkBasedOnEdges(g, partitionArray, n, m, world_size, world_rank);

  timer total_timer;
  total_timer.start();

  switch (strategy)
  {
  case 0:
    triangleCountSerial(g);
    break;

  case 1:
    triangleCountMPI(g, world_rank, partitionArray);
    break;
  }

  if (world_rank == ROOT_PROCESS)
  {
    double total_time = total_timer.stop();
    std::printf("Time taken (in seconds) : %f\n", total_time);
  }

  MPI_Finalize();
  delete[] partitionArray;
  return 0;
}
