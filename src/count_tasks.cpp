#include "precompiled_stl.hpp"

using namespace std;

#include "utils.hpp"
#include "core_functions.hpp"
#include "image_functions.hpp"
#include "visu.hpp"
#include "read.hpp"

// =========
// count_tasks.cpp
// count the number of tests
// =========

int main() {
  vector<Sample> sample = readAll("test", -1);
  cout << sample.size() << endl;
}
