#include "solution.h"

void *producerFunction(void *_arg)
{
  // Parse the _arg passed to the function.
  // Enqueue `n` items into the `production_buffer`. The items produced should
  // be 0, 1, 2,..., (n-1).
  // Keep track of the number of items produced and the value produced by the
  // thread.
  // The producer that was last active should ensure that all the consumers have
  // finished. NOTE: Each thread will enqueue `n` items.
  // Use mutex variables and conditional variables as necessary.
  // Each producer enqueues `n` items

  // reference: https://www.youtube.com/watch?v=ynCc-v0K-do

  timer t;
  t.start();
  producer_args *prod_args = (producer_args *)_arg;

  CircularQueue *production_buffer = prod_args->production_buffer;
  int producer_id = prod_args->producer_id;
  long *n_items = prod_args->n_items;
  // long *items = prod_args->items;
  long *items = new long(0);
  // long *items = prod_args->items_produced;
  // std::cout << "producer n_items " << *n_items << "\n";
  long *items_produced = prod_args->items_produced;
  // std::cout << "producer items_produced " << *items_produced << "\n";
  long *value_produced = prod_args->value_produced;
  // std::cout << "producer value_produced " << *value_produced << "\n";
  int *active_producer_count = prod_args->active_producer_count;
  int *active_consumer_count = prod_args->active_consumer_count;
  double *time_taken_producer = prod_args->time_taken_producer;

  pthread_mutex_t *mutex = prod_args->mutex;
  pthread_cond_t *buffer_not_full = prod_args->buffer_not_full;
  pthread_cond_t *buffer_not_empty = prod_args->buffer_not_empty;

  while (*items_produced < *n_items)
  {
    pthread_mutex_lock(mutex);

    // Check if the production_buffer is full.
    // while (production_buffer->isFull())
    // {
    //   // if (*active_consumer_count == 0)
    //   // {
    //   //   // If there are no more active consumers, then exit.
    //   //   pthread_mutex_unlock(mutex);
    //   //   pthread_exit(NULL);
    //   // }
    //   // If it's full, the producer should wait on a conditional variable
    //   // until a consumer signals that it's safe to enqueue items.
    //   pthread_cond_wait(buffer_not_full, mutex);
    // }

    bool ret = production_buffer->enqueue(*items);
    if (ret == true)
    {
      // *value_produced += *n_items;
      // (*items_produced)++;
      // (*n_items)++;
      if (production_buffer->itemCount() == 1)
      {
        // The queue is no longer empty
        // Signal all consumers indicating queue is not empty
        pthread_cond_broadcast(buffer_not_empty);
      }
      *value_produced += *items;
      (*items_produced)++;
      (*items)++;

    // std::cout << "Producer " << producer_id << " produced " << *items_produced << " item " << *items << "  n_items "<< *n_items << "\n";

      pthread_mutex_unlock(mutex);
    }
    else
    {
      // production_buffer is full, so block on conditional variable waiting for consumer to signal.
      pthread_cond_wait(buffer_not_full, mutex);
      pthread_mutex_unlock(mutex);
    }
  }

  // After production is completed:
  pthread_mutex_lock(mutex);
  // Update the number of producers that are currently active.
  --*active_producer_count;

  // The producer that was last active (can be determined using `active_producer_count`) will keep signalling the consumers until all consumers have finished (can be determined using `active_consumer_count`).
  // if (*active_producer_count == 0 || *active_consumer_count == 0)
   if (*active_producer_count == 0)
  {
    // If this is the last active producer, signal all consumers that no more items will be produced.
    // std::cout << "Producer " << producer_id << " signalling consumers\n";
    pthread_cond_broadcast(buffer_not_empty);
  }

  pthread_mutex_unlock(mutex);

  *time_taken_producer = t.stop();
  pthread_exit(_arg);
}

void *consumerFunction(void *_arg)
{
  // Parse the _arg passed to the function.
  // The consumer thread will consume items by dequeueing the items from the
  // `production_buffer`.
  // Keep track of the number of items consumed and the value consumed by the
  // thread.
  // Once the productions is complete and the queue is also empty, the thread
  // will exit. NOTE: The number of items consumed by each thread need not be
  // same.
  // Use mutex variables and conditional variables as necessary.
  timer t;
  t.start();
  consumer_args *cons_args = (consumer_args *)_arg;
  CircularQueue *production_buffer = cons_args->production_buffer;

  // long *item = cons_args->items;
  long item;
  long *items_consumed = cons_args->items_consumed;
  long *value_consumed = cons_args->value_consumed;
  int *active_consumer_count = cons_args->active_consumer_count;
  int *active_producer_count = cons_args->active_producer_count;
  double *time_taken_consumer = cons_args->time_taken_consumer;

  pthread_mutex_t *mutex = cons_args->mutex;
  pthread_mutex_t *active_consumer_count_mutex = cons_args->active_consumer_count_mutex;
  pthread_cond_t *buffer_not_full = cons_args->buffer_not_full;
  pthread_cond_t *buffer_not_empty = cons_args->buffer_not_empty;

  while (true)
  {
    pthread_mutex_lock(mutex);

    // Check if the production_buffer is empty.
    while (production_buffer->isEmpty())
    {
      if (*active_producer_count == 0)
      {
        // If there are no more active producers, and the queue is empty, then exit.
        pthread_mutex_unlock(mutex);
        // std::cout << "Consumer " << cons_args->items_consumed << " exiting\n";
        *time_taken_consumer = t.stop();
        pthread_exit(_arg);
      }
      // If it's empty, the consumer should wait on a conditional variable
      // until a producer signals that it's safe to dequeue items.
      // std::cout << "Before consumer " << cons_args->items_consumed << " waiting\n";
      pthread_cond_wait(buffer_not_empty, mutex);
      // std::cout << "Consumer " << cons_args->items_consumed << " waiting\n";
    }

    bool ret = production_buffer->dequeue(&item);
    if (ret)
    {
      // std::cout << "Consumer " << cons_args->items_consumed << " consumed " << *n_items << "\n";
      *value_consumed += item;
      (*items_consumed)++;
      if (production_buffer->itemCount() == production_buffer->getCapacity() - 1)
      {
        // The queue is no longer full
        // Signal all producers indicating queue is not full
        pthread_cond_broadcast(buffer_not_full);
      }
      
    }
    else
    {
      // production_buffer is empty, so block on conditional variable waiting for producer to signal.
      // The thread can wake up because of 2 scenarios:
      // Scenario 1: There are no more active producers (i.e., production is complete) and the queue is empty. This is the exit condition for consumers, and at this point consumers should decrement `active_consumer_count`.
      // Scenario 2: The queue is not empty and/or the producers are active. Continue consuming.
      pthread_cond_wait(buffer_not_empty, mutex);
    }
    pthread_mutex_unlock(mutex);
  }
  *time_taken_consumer = t.stop();
  pthread_exit(_arg);
}

ProducerConsumerProblem::ProducerConsumerProblem(long _n_items,
                                                 int _n_producers,
                                                 int _n_consumers,
                                                 long _queue_size)
    : n_items(_n_items), n_producers(_n_producers), n_consumers(_n_consumers),
      production_buffer(_queue_size)
{
  std::cout << "Constructor\n";
  std::cout << "Number of items: " << n_items << "\n";
  std::cout << "Number of producers: " << n_producers << "\n";
  std::cout << "Number of consumers: " << n_consumers << "\n";
  std::cout << "Queue size: " << _queue_size << "\n";

  producer_threads = new pthread_t[n_producers];
  consumer_threads = new pthread_t[n_consumers];
  // Initialize all mutex and conditional variables here.
  producer_args_array = new producer_args[n_producers];
  consumer_args_array = new consumer_args[n_consumers];
  mutex = PTHREAD_MUTEX_INITIALIZER;
  buffer_not_full = PTHREAD_COND_INITIALIZER;
  buffer_not_empty = PTHREAD_COND_INITIALIZER;

  items_produced = new long[n_producers];
  items_consumed = new long[n_consumers];
  value_produced = new long[n_producers];
  value_consumed = new long[n_consumers];
  time_taken_producer = new double[n_producers];
  time_taken_consumer = new double[n_consumers];

  active_producer_count = 0;
  active_consumer_count = 0;

}

ProducerConsumerProblem::~ProducerConsumerProblem()
{
  std::cout << "Destructor\n";
  delete[] producer_threads;
  delete[] consumer_threads;
  delete[] items_produced;
  delete[] items_consumed;
  delete[] value_produced;
  delete[] value_consumed;
  delete[] time_taken_producer;
  delete[] time_taken_consumer;
  // Destroy all mutex and conditional variables here.
  delete[] producer_args_array;
  delete[] consumer_args_array;

  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&buffer_not_full);
  pthread_cond_destroy(&buffer_not_empty);
}

void ProducerConsumerProblem::startProducers()
{
  std::cout << "Starting Producers\n";
  active_producer_count = n_producers;
  // Create producer threads P1, P2, P3,.. using pthread_create.
  for (int i = 0; i < n_producers; i++)
  {
    producer_args *prod_args = new producer_args();
    prod_args->production_buffer = &production_buffer;
    prod_args->producer_id = i;
    prod_args->n_items = &n_items;
    // prod_args->items = 0;
    prod_args->items = new long(0);
    items_produced[i] = 0;
    prod_args->items_produced = &items_produced[i];
    value_produced[i] = 0;
    prod_args->value_produced = &value_produced[i];
    prod_args->active_producer_count = &active_producer_count;
    prod_args->active_consumer_count = &active_consumer_count;
    prod_args->time_taken_producer = &time_taken_producer[i];

    prod_args->mutex = &mutex;
    prod_args->buffer_not_full = &buffer_not_full;
    prod_args->buffer_not_empty = &buffer_not_empty;

    int err = pthread_create(&producer_threads[i], NULL, producerFunction, (void *)prod_args);
    if (err != 0)
    {
      printf("\ncan't create thread :[%s]", strerror(err));
    }
  }


}

void ProducerConsumerProblem::startConsumers()
{
  std::cout << "Starting Consumers\n";
  active_consumer_count = n_consumers;
  // Create consumer threads C1, C2, C3,.. using pthread_create.

  for (int i = 0; i < n_consumers; i++){
    consumer_args *cons_args = new consumer_args();
    cons_args->production_buffer = &production_buffer;
    cons_args->items = &n_items;
    items_consumed[i] = 0;
    cons_args->items_consumed = &items_consumed[i];
    cons_args->value_consumed = &value_consumed[i];
    cons_args->active_consumer_count = &active_consumer_count;
    cons_args->active_producer_count = &active_producer_count;
    cons_args->time_taken_consumer = &time_taken_consumer[i];

    cons_args->mutex = &mutex;
    cons_args->buffer_not_full = &buffer_not_full;
    cons_args->buffer_not_empty = &buffer_not_empty;

    int err = pthread_create(&consumer_threads[i], NULL, consumerFunction, (void *)cons_args);
    if (err != 0)
    {
      printf("\ncan't create thread :[%s]", strerror(err));
    }
  }
}

void ProducerConsumerProblem::joinProducers()
{
  std::cout << "Joining Producers\n";
  // Join the producer threads with the main thread using pthread_join

  for (int i = 0; i < n_producers; i++)
  {
    int err = pthread_join(producer_threads[i], NULL);
    if (err != 0)
    {
      printf("\ncan't join thread :[%s]", strerror(err));
    }
  }
}

void ProducerConsumerProblem::joinConsumers()
{
  std::cout << "Joining Consumers\n";
  // Join the consumer threads with the main thread using pthread_join

  for (int i = 0; i < n_consumers; i++)
  {
    int err = pthread_join(consumer_threads[i], NULL);
    if (err != 0)
    {
      printf("\ncan't join thread :[%s]", strerror(err));
    }
  }
}

void ProducerConsumerProblem::printStats()
{
  std::cout << "Producer stats\n";
  std::cout << "producer_id, items_produced, value_produced, time_taken \n";
  // Make sure you print the producer stats in the following manner
  // 0, 1000, 499500, 0.00123596
  // 1, 1000, 499500, 0.00154686
  // 2, 1000, 499500, 0.00122881
  long total_produced = 0;
  long total_value_produced = 0;
  for (int i = 0; i < n_producers; i++)
  {
    std::cout << i << ", " << items_produced[i] << ", " << value_produced[i]
              << ", " << time_taken_producer[i] << "\n";
    total_produced += items_produced[i];
    total_value_produced += value_produced[i];
  }
  std::cout << "Total produced = " << total_produced << "\n";
  std::cout << "Total value produced = " << total_value_produced << "\n";
  std::cout << "Consumer stats\n";
  std::cout << "consumer_id, items_consumed, value_consumed, time_taken \n";
  // Make sure you print the consumer stats in the following manner
  // 0, 677, 302674, 0.00147414
  // 1, 648, 323301, 0.00142694
  // 2, 866, 493382, 0.00139689
  // 3, 809, 379143, 0.00134516
  long total_consumed = 0;
  long total_value_consumed = 0;
  for (int i = 0; i < n_consumers; i++)
  {
    std::cout << i << ", " << items_consumed[i] << ", " << value_consumed[i]
              << ", " << time_taken_consumer[i] << "\n";
    total_consumed += items_consumed[i];
    total_value_consumed += value_consumed[i];
  }
  std::cout << "Total consumed = " << total_consumed << "\n";
  std::cout << "Total value consumed = " << total_value_consumed << "\n";
}