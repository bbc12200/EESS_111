#ifndef __P1_PROCESS
#define __P1_PROCESS

#include <vector>

// Student struct
struct student {
  unsigned long id;
  double grade;

  student(unsigned long id, double grade) {
    this->id = id;
    this->grade = grade;
  }
};

void create_processes_and_sort(std::vector<std::string>, int, int);

#endif
