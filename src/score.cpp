#include "precompiled_stl.hpp"
using namespace std;
#include "utils.hpp"
#include "core_functions.hpp"
#include "image_functions.hpp"

#include "brute2.hpp"
#include "pieces.hpp"
#include "compose2.hpp"

// =========
// score.cpp
// determine if submission/program is correct
// =========

// return 1 if 1 of generated candidate grids is correct
int scoreCands(const vector<Candidate>&cands, Image_ test_in, Image_ test_out) {
  for (const Candidate&cand : cands)
    if (cand.imgs.back() == test_out) return 1;
  return 0;
}

// return 1 if 1 of submitted grids is correct
int scoreAnswers(vImage_ answers, Image_ test_in, Image_ test_out) {
  assert(answers.size() <= 3);
  for (Image_ answer : answers)
    if (answer.size == test_out.size && answer.mask == test_out.mask) return 1;
  return 0;
}
