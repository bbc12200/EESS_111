#ifndef __P1_THREADS
#define __P1_THREADS

#include <vector>
#include <pthread.h>

#include "p1_process.h"

// Class to handle multithreaded merge sort
class ParallelMergeSorter {
  private:
    std::vector<pthread_t> threads;
    std::vector<student> sorted_list;
    int num_threads;

    static void * thread_init(void *);

    void merge_sort(int, int);
    void merge(int, int, int);
    void merge_threads();
  public:
    ParallelMergeSorter(std::vector<student> &, int);

    std::vector<student> run_sort();
};

#endif
