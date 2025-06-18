#include <vector>
#include <pthread.h>
#include <cstring>
#include <string>
#include <cstdlib>
  
#include "p1_process.h"
#include "p1_threads.h"

using namespace std;

#include <stdio.h>
// This struct is used to pass arguments into each thread (inside of thread_init)
// ctx is essentially the "this" keyword, but since we are in a static function, we cannot use "this"
//
// Feel free to modify this struct in any way
struct MergeSortArgs {
    int thread_index;
    ParallelMergeSorter * ctx;
  
    MergeSortArgs(ParallelMergeSorter * ctx, int thread_index) {
      this->ctx = ctx;
      this->thread_index = thread_index;
    }
  };


// Class constructor
ParallelMergeSorter::ParallelMergeSorter(vector<student> &original_list, int num_threads) {
  this->threads = vector<pthread_t>();
  this->sorted_list = vector<student>(original_list);
  this->num_threads = num_threads;
}

// This function will be called by each child process to perform multithreaded sorting
vector<student> ParallelMergeSorter::run_sort()
{
    /*
     * Uncomment this code for testing sorting without threads
     * 
     * thread_init((void *) args);
     */


    // Your implementation goes here, you will need to implement:
    // Creating worker threads
    //  - Each worker thread will use ParallelMergeSorter::thread_init as its start routine
    //
    //  - Since the args are taken in as a void *, you will need to use
    //  the MergeSortArgs struct to pass multiple parameters (pass a pointer to the struct)
    //
    //  - Don't forget to make sure all threads are done before merging their sorted sublists
    

    // Build Threads
    for (int i = 0; i < num_threads; ++i) {
        MergeSortArgs *args = new MergeSortArgs(this, i);
        pthread_t tid;
        int ret = pthread_create(&tid, NULL, thread_init, args);
        if (ret != 0) {
            printf("thread_create \n");
            exit(1);
        }
        threads.push_back(tid);
    }
    // Wait thread finish
    for (size_t i = 0; i < threads.size(); ++i)
        pthread_join(threads[i], NULL);

    // sort the output of threads
    merge_threads();

    return sorted_list;
}

// Standard merge implementation for merge sort
void ParallelMergeSorter::merge_sort(int lower, int upper){

    // Your implementation goes here, you will need to implement:
    // Top-down merge sort
    if (upper - lower <= 1) {
        return;
    }
    int middle = lower + (upper - lower) / 2;
    merge_sort(lower, middle);
    merge_sort(middle, upper);
    merge(lower, middle, upper);
}


// Standard merge implementation for merge sort
void ParallelMergeSorter::merge(int lower, int middle, int upper){
    // Your implementation goes here, you will need to implement:
    // Merge for top-down merge sort
    //  - The merge results should go in temporary list, and once the merge is done, the values
    //  from the temporary list should be copied back into this->sorted_list
    
    int merge_size = upper - lower;
    vector<student> temp;
    temp.reserve(merge_size);

    int i = lower, j = middle;
    while (i < middle && j < upper) {
        const student &a = sorted_list[i];
        const student &b = sorted_list[j];
    
        if (a.grade > b.grade || (a.grade == b.grade))
        {
            temp.push_back(a);
            ++i;
        } else {
            temp.push_back(b);
            ++j;
        }
    }
    while (i < middle) {
        temp.push_back(sorted_list[i++]);
    }
    while (j < upper) {
        temp.push_back(sorted_list[j++]);
    }

    for (int k = 0; k < merge_size; ++k){
        sorted_list[lower + int(k)] = temp[k];
    }

}

// This function will be used to merge the resulting sorted sublists together
void ParallelMergeSorter::merge_threads(){
    // Your implementation goes here, you will need to implement:
    // Merging the sorted sublists together
    //  - Each worker thread only sorts a subset of the entire list, therefore once all
    //  worker threads are done, we are left with multiple sorted sublists which then need to
    //  be merged once again to result in one total sorted list
    int array_length = (int)sorted_list.size();
    
    int* boundaries = new int[num_threads + 1];
    
    int work_per_thread = array_length / num_threads;


    for (int i = 0; i < num_threads; ++i) {
        boundaries[i] = i * work_per_thread;
    }
    boundaries[num_threads] = array_length;
    
    int current_segments = num_threads;
    
    while (current_segments > 1) {
        int new_segments = 0;
        
        for (int i = 0; i < current_segments; i += 2) {
            if (i + 1 < current_segments) {
                int left = boundaries[i];
                int mid = boundaries[i + 1];
                int right = boundaries[i + 2];
                
                merge(left, mid, right);
                
                boundaries[new_segments] = left;
            } else {
                boundaries[new_segments] = boundaries[i];
            }
            new_segments++;
        }
        
        boundaries[new_segments] = boundaries[current_segments];
        current_segments = new_segments;
    }
    
    delete[] boundaries;
}
// This function is the start routine for the created threads, it should perform merge sort on its assigned sublist
// Since this function is static (pthread_create must take a static function), we cannot access "this" and must use ctx instead
void *ParallelMergeSorter::thread_init(void *args){
    MergeSortArgs * sort_args = (MergeSortArgs *) args;
    int thread_index = sort_args->thread_index;
    ParallelMergeSorter * ctx = sort_args->ctx;
  
    int work_per_thread = ctx->sorted_list.size() / ctx->num_threads;
 
    printf("Thread Index:%d \n", thread_index);

    // Your implementation goes here, you will need to implement:
    // Getting the sublist to sort based on the thread index
    //  - The lower bound is thread_index * work_per_thread, the upper bound is (thread_index + 1) * work_per_thread
    //
    //  - The range of merge sort is typically [lower, upper), lower inclusive, upper exclusive
    //
    // We have to consider the how many data the last thread needs to compute
    // Sometimes, the number of threads cannot divide evenly by the amount of data to be processed. 
    // The last one process the remaingin data
    int lower = (thread_index) * work_per_thread;
    int upper;
    if (thread_index == (ctx->num_threads - 1)) {
        upper = ctx->sorted_list.size();
    } else {
        upper = (thread_index + 1) * work_per_thread;
    }
    
    //  - Remember to make sure all elements are included in the sort, integer division rounds down
    //

    // Running merge sort
    //  - The provided functions assume a top-down implementation
    //
    //  - Many implementations of merge sort are online, feel free to refer to them (wikipedia is good)
    //
    //  - It may make sense to equivalate this function as the non recursive "helper function" that merge sort normally has

    // Free the heap allocation

    ctx->merge_sort(lower, upper);

    delete sort_args;
    return NULL;
}
