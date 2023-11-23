#include <iostream>
#include <cstdio>
#include "core/utils.h"
#include "core/graph.h"

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

void assignVertices(Graph &g, uintV* verticesArray, uintV n, uintE m, uint processes, uint global_rank)
{
  uintE edges_per_process = m / processes;
  uint start_vertex = 0, end_vertex = 0;
	int cur_process = 0;
	// verticesArray[processes] = n;

  for (int i = 0; i < processes; i++)
  {
    start_vertex = end_vertex;
    uintE count = 0; // Count of edges for this process

    while (end_vertex < n)
    {
      // add vertices unil we reach m/P edges
      count += g.vertices_[end_vertex].getOutDegree();
      end_vertex++;

      // Check if this process has enough edges or if we've covered all vertices
      if (count >= edges_per_process)
      {
        verticesArray[i] = end_vertex;
        break;
      }
    }
    if (i == global_rank)
    {
      break;
    }

    // Handling the case where the last process might have a few extra edges
    if (global_rank == processes - 1) {
        verticesArray[processes - 1] = n;
    }
  }
}

void triangleCountMPI(Graph &g, uint processes, uint rank)
{
  uint n = g.n_;
  uint processes = g.m_;
  uint start_vertex = 0, end_vertex = 0;

  const int edges_per_process = n / processes;
  int remainder = n % processes;
  long local_count = 0, global_count = 0;

  // Initialize the MPI environment
  MPI_Status status;
  MPI_Init(NULL, NULL);

  int global_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &global_rank);

  for (int i; i < processes; i++)
  {
    start_vertex = end_vertex;
    long count = 0;
    while (end_vertex < n)
    {
      // add vertices unil we reach m/P edges
      count += g.vertices_[end_vertex].getOutDegree();
      end_vertex++;
      if (count >= edges_per_process)
      {
        break;
      }
    }
    if (i == global_rank)
    {
      break;
    }
  }

  // Each process will work on vertices [start_vertex, end_vertex).
  for (int p; p < processes; p++)
  {
    long local_count = 0;
    // for each vertex 'u' allocated to P
    for (uintV u = start_vertex; u < end_vertex; u++)
    {
      uintE out_degree = g.vertices_[u].getOutDegree();
      uintE edges_processed = 0; // // used in output validation
      edges_processed += out_degree;

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
    // if(P is root process){
    //     global_count = Sum of local counts of all the processes
    // }
    // else{
    // depending on the strategy,
    // use appropriate API to send the local_count to the root process
    // }

    if (global_rank == ROOT_PROCESS)
    {
      global_count += local_count;
    }
    else
    {
      MPI_Send(&local_count, 1, MPI_LONG, ROOT_PROCESS, 0, MPI_COMM_WORLD);
    }

    // --- synchronization phase end -----
    // if(P is root process){
    //     global_count = global_count / 3
    //     // print process statistics and other results
    // }
    // else{
    //     // print process statistics
    // }
    if (global_rank == ROOT_PROCESS)
    {
      global_count = global_count / 3;
      std::printf("Number of triangles : %ld\n", global_count);
      std::printf("Number of unique triangles : %ld\n", global_count / 3);
      std::printf("Time taken (in seconds) : %f\n", time_taken);
      MPI_Finalize();
      return
    }
    else
    {
      std::printf("Number of triangles : %ld\n", local_count);
      std::printf("Number of unique triangles : %ld\n", local_count / 3);
      std::printf("Time taken (in seconds) : %f\n", time_taken);
      MPI_Finalize();
      return;
    }
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
  MPI_Init(&argc, &argv);

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
  uintV* verticesArray = new uintV[world_rank];
  assignVertices(g, verticesArray, n, m, world_size, world_rank);

  switch (strategy)
  {
  case 0:
    triangleCountSerial(g);
    break;

  case 1:
    triangleCountMPI(g, world_size, world_rank);
    break;
  }

  MPI_Finalize();

  return 0;
}
