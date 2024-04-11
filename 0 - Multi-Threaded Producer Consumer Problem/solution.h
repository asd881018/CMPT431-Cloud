#include "core/circular_queue.h"
#include "core/utils.h"
#include <pthread.h>
#include <stdlib.h>

typedef struct
{
  CircularQueue *production_buffer;
  int producer_id;
  long *n_items;
  long *items;
  long *items_produced;
  long *value_produced;
  int *active_producer_count;
  int *active_consumer_count;
  double *time_taken_producer;

  pthread_mutex_t *mutex;
  pthread_cond_t *buffer_not_full;
  pthread_cond_t *buffer_not_empty;

} producer_args;

typedef struct
{
  CircularQueue *production_buffer;
  long *items;
  long *items_consumed;
  long *value_consumed;
  int *active_consumer_count;
  int *active_producer_count;
  double *time_taken_consumer;

  pthread_mutex_t *mutex;
  pthread_mutex_t *active_consumer_count_mutex;
  pthread_cond_t *buffer_not_full;
  pthread_cond_t *buffer_not_empty;

} consumer_args;

class ProducerConsumerProblem {
  long n_items;
  long *items;
  int n_producers;
  int n_consumers;
  CircularQueue production_buffer;

  // Dynamic array of thread identifiers for producer and consumer threads.
  // Use these identifiers while creating the threads and joining the threads.
  pthread_t *producer_threads;
  pthread_t *consumer_threads;

  int active_producer_count;
  int active_consumer_count;
  
  pthread_mutex_t mutex;
  pthread_cond_t buffer_not_full;
  pthread_cond_t buffer_not_empty;

  consumer_args *consumer_args_array;
  producer_args *producer_args_array;

  long *items_produced;
  long *items_consumed;
  long *value_produced;
  long *value_consumed;
  double *time_taken_producer;
  double *time_taken_consumer;

public:
  // The following 6 methods should be defined in the implementation file (solution.cpp)
  ProducerConsumerProblem(long _n_items, int _n_producers, int _n_consumers,
                          long _queue_size);
  ~ProducerConsumerProblem();
  void startProducers();
  void startConsumers();
  void joinProducers();
  void joinConsumers();
  void printStats();
};
