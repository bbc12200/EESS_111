#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <unistd.h>
#include <string>

#include "p1_process.h"
#include "p1_threads.h"

using namespace std;

int main(int argc, char** argv) {
  printf("Main process is created. (pid: %d)\n", getpid());
  int num_processes = 0;
  int num_threads = 0;

  vector<string> class_name;
  class_name.push_back("os");
  class_name.push_back("architecture");
  class_name.push_back("java");
  class_name.push_back("algorithm");
  class_name.push_back("digital-design");

  // Check the argument and print error message if the argument is wrong
  if(argc == 3 && (atoi(argv[1]) > 0 && atoi(argv[2]) > 0))
  {
      num_processes = atoi(argv[1]);
      num_threads = atoi(argv[2]);
      
      // Create the child processes and sort
      create_processes_and_sort(class_name, num_processes, num_threads);
  }
  else
  {
      printf("[ERROR] Expecting 2 arguments with integral value greater than zero.\n");
      printf("[USAGE] %s <number of processes> <number of threads>\n", argv[0]);
  }
  printf("Main process is terminated. (pid: %d)\n", getpid());
  return 0;
}

