#include "precompiled_stl.hpp"
using namespace std;

#include "utils.hpp"
#include "core_functions.hpp"
#include "image_functions.hpp"
#include "image_functions2.hpp"
#include "visu.hpp"

#include "brute2.hpp"
#include "pieces.hpp"
#include "compose2.hpp"

extern int MAXDEPTH;

/**
 * compare points (object with x and y property, could represent grid size) (defines what < means thru operator overloading)
 * https://www.tutorialspoint.com/cplusplus/cpp_overloading.htm
 * @param a point before operator
 * @param b point after operator
 * @returns bool
 */
bool operator<(const point a, const point b) {
  if (a.x != b.x) return a.x < b.x;
  return a.y < b.y;
}

/**
 brute force solve a single dimension of the output grid
 @param seeds input grid sizes of dimension
 @param target correct output grid sizes of dimension
 @returns best size prediction and the predictions loss
 */
pair<vector<int>,double> solveSingleDimension(vector<vector<int>>&seeds, const vector<int>& target) {

  int n = target.size()+1;
  vector<int> ans(n,1);
  pair<int,double> best = {-1,1e9};

  // anonymous function to update dimension size guess if better than current best
  auto add = [&](const vector<int>&szs, double loss) {

    // calculate number of sizes that are correct for train IOs
    int oks = 0;
    for (int ti = 0; ti < target.size(); ti++)
      oks += (szs[ti] == target[ti]);

    // store fitness of grid size prediction
    pair<int,double> cand = {oks, -loss-10};
    int test_output_sz = szs.back();

    // update grid size guess if grid size is within ARC bounds (1 and 30) and better than current best
    if (test_output_sz >= 1 && test_output_sz <= 30 && cand > best) {
      best = cand;
      ans = szs;
    }
  };

  // update answer to have best grid size by bruteforcing every possible width
  for (int w = 1; w <= 30; w++) {
    add(vector<int>(n,w), w);
  }

  // search multipliers 1 - 6 and offset -3 - 3 for each seed
  vector<int> szs(n);
  for (int i = 0; i < seeds.size(); i++) {
    double a = i+1;
    for (int w = 1; w < 6; w++) {
      for (int x = -3; x <= 3; x++) {
        for (int k = 0; k < n; k++)
          szs[k] = seeds[i][k]*w+x;
        add(szs, a*w*(abs(x)+1));
      }
    }
  }

  // return best size prediction and the loss of the prediction
  return {ans,best.second};
}

/**
 brute force solve both dimensions of the output grid
 @param seeds input grid sizes
 @param target correct output grid sizes
 @returns best grid sze
 */
point solveSize(vector<vector<point>>&seeds, const vector<point>& target) {

  point ans = {1,1};
  pair<int,double> best = {-1,1e9};

  // anonymous function to update grid size guess if better than current best
  auto add = [&](const vector<point>&szs, double loss) {
    int oks = 0;
    for (int ti = 0; ti < target.size(); ti++)
      oks += (szs[ti] == target[ti]);
    pair<int,double> cand = {oks, -loss};
    point sz = szs.back();
    if (sz.x >= 1 && sz.x <= 30 &&
        sz.y >= 1 && sz.y <= 30 &&
        cand > best) {
      best = cand;
      ans = sz;
    }
  };

  int n = target.size()+1;
  vector<point> szs(n);

  // try a simple multiplier and offset for each dimension
  for (int i = 0; i < seeds.size(); i++) {
    double a = i+1;
    for (int h = 1; h < 6; h++) {
      for (int w = 1; w < 6; w++) {
        for (int y = -3; y <= 3; y++) {
          for (int x = -3; x <= 3; x++) {
            for (int k = 0; k < n; k++)
	            szs[k] = {seeds[i][k].x*w+x, seeds[i][k].y*h+y};
	          add(szs, a*w*h*(abs(x)+1)*(abs(y)+1));
	        }
	      }
      }
    }
  }

  // try some non-linear transformations
  if (1) {//best.first < target.size()) {
    for (int i = 0; i < seeds.size(); i++) {
      double a = i+1;
      for (int j = 0; j < i; j++) {
	      double b = j+1;
	      for (int d = 0; d < 3; d++) {
	        for (int k = 0; k < n; k++) {
	          szs[k] = seeds[i][k];
            if (d == 0 || d == 2) szs[k].x = szs[k].x+seeds[j][k].x;
            if (d == 1 || d == 2) szs[k].y = szs[k].y+seeds[j][k].y;
          }
	      add(szs, a*b);
	      }
      }
    }
  }

  // random { } is to introduce new scope
  // solve each dimension individually, try those
  {
    vector<vector<int>> seedsx, seedsy;
    for (vector<point>&seed : seeds) {
      vector<int> seedx, seedy;
      for (point p : seed) {
        seedx.push_back(p.x);
        seedy.push_back(p.y);
      }
      seedsx.push_back(seedx);
      seedsy.push_back(seedy);
    }
    vector<int> targetx, targety;
    for (point p : target) {
      targetx.push_back(p.x);
      targety.push_back(p.y);
    }
    auto [bestx, scorex] = solveSingleDimension(seedsx, targetx);
    auto [besty, scorey] = solveSingleDimension(seedsy, targety);

    vector<point> combined;
    for (int i = 0; i < n; i++) {
      combined.push_back({bestx[i], besty[i]});
    }
    double combloss = scorex*scorey;
    add(combined, combloss);
  }

  return ans;
}

/**
 brute force solve grid sizes for every IO
 @param test_in test input
 @param train train IOs
 @returns predicted grid size of each IO
 */
vector<point> bruteSize(Image_ test_in, vector<pair<Image,Image>> train) {

  vector<point> out_sizes;
  for (auto [in,out] : train) out_sizes.push_back(out.sz);

  // get objects from training IOs and test input
  int unconstrained_max_depth = MAXDEPTH;
  MAXDEPTH = min(MAXDEPTH, 30);
  Pieces pieces;
  {
    vector<DAG> dags = brutePieces2(test_in, train, {});
    pieces = makePieces2(dags, train, {});
  }
  int dags = pieces.dag.size();

  MAXDEPTH = unconstrained_max_depth;

  // target is the grid output sizes (copy of output_sizes)
  vector<point> target;
  for (auto [in,out] : train) target.push_back(out.sz);

  // not sure -- will circle back after looking through Piece code
  // TODO: leave better comment
  vector<vector<point>> seeds;
  set<vector<point>> seen;
  for (Piece3&p : pieces.piece) {
    vector<point> sz;
    int ok = 1;
    int*ind = &pieces.mem[p.memi];
    for (int ti = 0; ti <= train.size(); ti++) {
      if (pieces.dag[ti].tiny_node[ind[ti]].isvec) ok = 0;
      else sz.push_back(pieces.dag[ti].getImg(ind[ti]).sz);
    }
    if (ok) {
      if (seen.insert(sz).second)
	      seeds.push_back(sz);
    }
  }

  // solve for test output size, return all output sizes 
  point ans = solveSize(seeds, target);
  out_sizes.push_back(ans);
  return out_sizes;
}
