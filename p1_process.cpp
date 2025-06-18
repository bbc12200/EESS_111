#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <cstdio>
#include <cmath>
#include <unistd.h>
#include <sys/wait.h>

#include "p1_process.h"
#include "p1_threads.h"

using namespace std;

// This file implements the multi-processing logic for the project


// This function should be called in each child process right after forking
// The input vector should be a subset of the original files vector
void process_classes(vector<string> classes, int num_threads) {
  printf("Child process is created. (pid: %d)\n", getpid());
  // Each process should use the sort function which you have defined  		
  // in the p1_threads.cpp for multithread sorting of the data. 
  printf("This child Process is processing: ");
  for (size_t i = 0; i < classes.size(); ++i) {
    printf("%s. ", classes[i].c_str());
  }
  printf("\n");


  for (int i = 0; i < classes.size(); i++) {
    // get all the input/output file names here
    string class_name = classes[i];
    char buffer[40];

    sprintf(buffer, "input/%s.csv", class_name.c_str());
    string input_file_name(buffer);

    sprintf(buffer, "output/%s_sorted.csv", class_name.c_str());
    string output_sorted_file_name(buffer);

    sprintf(buffer, "output/%s_stats.csv", class_name.c_str());
    string output_stats_file_name(buffer);

    vector<student> students;
    // Your implementation goes here, you will need to implement:
    // File I/O
    //  - This means reading the input file, and creating a list of students,
    //  see p1_process.h for the definition of the student struct
    //
    FILE* input_file = fopen(input_file_name.c_str(), "r");
    // Read Header
    char header[128];
    fgets(header, sizeof(header), input_file);

    // Read Each line data
    char line[128];
    while (fgets(line, sizeof(line), input_file)) {
        unsigned long id;
        double score;

        if (sscanf(line, "%lu,%lf", &id, &score) == 2) {
            student s(id,score);
            students.push_back(s);
        }
    }
    fclose(input_file);
    printf("%s, student amount: %ld \n",class_name.c_str(), students.size());
  
    
    //  - Also, once the sorting is done and the statistics are generated, this means
    //  creating the appropritate output files
    //
    // Multithreaded Sorting
    //  - See p1_thread.cpp and p1_thread.h
    //
    //  - The code to run the sorter has already been provided
    //
    // Generating Statistics
    //  - This can be done after sorting or during

    // Run multi threaded sort
    ParallelMergeSorter sorter(students, num_threads);
    vector<student> sorted = sorter.run_sort();
    //vector<student> sorted = students;

    double Average = 0.0;
    double Median = 0.0;
    double Variance = 0.0;
    double Std_Dev = 0.0;
    double Temp_sum = 0.0;

    FILE* output_sorted_file = fopen(output_sorted_file_name.c_str(), "w");
    if (!output_sorted_file) {
      perror(("Failed to open " + output_sorted_file_name).c_str());
      exit(1);
    }

    fprintf(output_sorted_file, "Rank,Student ID,Grade\n");
    int students_size = sorted.size();

    int n = 0;
    double M2 = 0.0;

    for (int i = 0; i< students_size;i++){
      const student &s = sorted[i];
      fprintf(output_sorted_file, "%d,%lu,%lf \n", (i+1), s.id, s.grade);
      
      n++;

      double delta = s.grade - Average;
      Average += delta / n;
      M2 += delta * (s.grade - Average);  // Welford's update
    }


    if (students_size % 2 == 0) {
      Median = (sorted[students_size / 2 - 1].grade + sorted[students_size / 2].grade) / 2.0;
    } else {
      Median = sorted[students_size / 2].grade;
    }
    Std_Dev = sqrt(M2 / students_size); 

    fclose(output_sorted_file);
    FILE* output_static_file = fopen(output_stats_file_name.c_str(), "w");
    fprintf(output_static_file, "Average,Median,Std. Dev\n");
    fprintf(output_static_file, "%.3lf,%.3lf,%.3lf\n", Average, Median, Std_Dev);
    fclose(output_static_file);

  }

  // child process done, exit the program
  printf("Child process is terminated. (pid: %d)\n", getpid());
  exit(0);
}

//num_processes : number of child process

void create_processes_and_sort(vector<string> class_names, int num_processes, int num_threads) {
  vector<pid_t> child_pids;
  
  vector<string> classes_sublist;
  int classes_per_process = max(class_names.size() / num_processes, 1ul);
  int classes_amount = class_names.size();
  int classes_remain = classes_amount;
  int current_index = 0;
  int basic_work = classes_amount/num_processes;
  int extra_work_index = classes_amount%num_processes;
  
  if(basic_work<1){
    basic_work = 1;
    extra_work_index = 0;
  }
  // Test: Make sure parameter is transmitted.
  /*
  printf("Class names:\n");
  for (size_t i = 0; i < class_names.size(); ++i) {
    printf("%s\n", class_names[i].c_str());
  }
  printf("num_processes: %d \n", num_processes);
  printf("num_threads: %d \n", num_threads);
  printf("classes_per_process: %d \n", classes_per_process);
  */
  // This code is provided to you to test your sorter, this code does not use any child processes
  // Remove this later on
  // process_classes(class_names, 1);
  for (int i = 0; i < num_processes; ++i) {
    // If there's no remaining data, stop creating new child processes
    if (classes_remain <= 0) {
        break;
    }
    // Calculate how many work items current process should take
    int count = basic_work;
    if (i < extra_work_index) {
        count += 1;  // First few processes take one extra work item
    }
    // Last safeguard: ensure not going out of bounds
    if (current_index + count > class_names.size()) {
        count = class_names.size() - current_index;
    }
    // If count is 0, this process has nothing to do, skip it
    if (count == 0) {
        continue;
    }
    // Create corresponding sublist
    vector<string> sublist(
        class_names.begin() + current_index,
        class_names.begin() + current_index + count
    );
    current_index += count;
    classes_remain -= count;
    // Create child process
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {
        // Child process: handle its own sublist
        process_classes(sublist, num_threads);
        exit(0);  // Child process exits after completion
    } else {
        // Parent process: record PID
        child_pids.push_back(pid);
    }
  }
  for (size_t i = 0; i < child_pids.size(); ++i) {
      waitpid(child_pids[i], NULL, 0);
  }
}
  // Your implementation goes here, you will need to implement:
  // Splitting up work
  //   - Split up the input vector of classes into num_processes sublists
  //
  //   - Make sure all classes are included, remember integer division rounds down
  //
  // Creating child processes
  //   - Each child process will handle one of the sublists from above via process_classes
  //   
  // Waiting for all child processes to terminate
  

