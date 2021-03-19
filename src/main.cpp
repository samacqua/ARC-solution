#include "runner.hpp"
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

// =========
// main.cpp
// file called by executable
// =========

int main(int argc, char**argv) {
  //rankFeatures();
  //evalNormalizeRigid();
  //evalTasks();
  //bruteSubmission();
  //bruteSolve();
  int single_task_id = -1;
  if (argc >= 2) {
    single_task_id = atoi(argv[1]);
    printf("Running only task # %d\n", single_task_id);
  }
  int maxdepth = -1;
  if (argc >= 3) {
    maxdepth = atoi(argv[2]) * 10;  // for some reason, max depth is encoded as 10x actual max depth
    printf("Using max depth %d\n", maxdepth);
  }
  bool training = true;
  if (argc >= 4) {
    training = argv[3];
    printf("On Training Dataset (t) or Validation (f) %d\n", training);
  }

  run(single_task_id, maxdepth, training);
}
