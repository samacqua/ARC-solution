#include "precompiled_stl.hpp"
#include <chrono>
using namespace std;
#include "utils.hpp"
#include "read.hpp"
#include "normalize.hpp"
#include "core_functions.hpp"
#include "image_functions.hpp"
#include "image_functions2.hpp"

#include "visu.hpp"

#include "brute2.hpp"
#include "pieces.hpp"
#include "timer.hpp"

extern int print_nodes;

/**
 * hash a vector
 * @param vec
 * @returns ull
 */
ull hashVec(const vector<int>&vec) {
  ull r = 1;
  for (int v : vec) {
    r = r*1069388789821391921+v;
  }
  return r;
}

/**
 * make pieces
 * @param dag
 * @param train
 * @param out_sizes
 * @returns Pieces
 */
Pieces makePieces(vector<DAG>&dag, vector<pair<Image,Image>> train, vector<point> out_sizes) {
  
  Timer set_time, piece_time, child_time;
  Pieces pieces;

  vector<int>&mem = pieces.mem;
  vector<int> depth_mem;

  int dags = dag.size();

  TinyHashMap seen;
  vector<queue<int>> q;

  // anonymous function
  auto add = [&](int d, const vector<int>&v) {
    assert(v.size() == dags);

    set_time.start();
    auto [memi, inserted] = seen.insert(hashVec(v), (int)mem.size());
    set_time.stop();

    if (inserted) {
      for (int i : v) {
	      mem.push_back(i);
      }
      depth_mem.push_back(d);
    }

    if (inserted || depth_mem[memi/dags] > d) {
      depth_mem[memi/dags] = d;
      while (q.size() <= d) q.push_back({});
      q[d].push(memi);
    }
  };

  for (int i = 0; i < dag[0].givens; i++) {
    vector<int> v(dags,i);
    add(dag[0].tiny_node[i].depth, v);
  }

  vector<vector<pair<int,int>>> slow_child(train.size()+1);
  vector<pair<int,vector<int>>> newi_list;

  piece_time.start();
  for (int depth = 0; depth < q.size(); depth++) {
    while (q[depth].size()) {
      int memi = q[depth].front();
      q[depth].pop();
      if (depth > depth_mem[memi/dags]) continue;
      assert(depth == depth_mem[memi/dags]);

      vector<int> ind(mem.begin()+memi, mem.begin()+memi+dags);

      {
        int ok = 1, maxdepth = -1;
        for (int i = 0; i < dags; i++) {
          maxdepth = max(maxdepth, (int)dag[i].tiny_node[ind[i]].depth);
          ok &= dag[i].tiny_node[ind[i]].ispiece;
        }
        if (ok && maxdepth >= depth) {
          Piece3 p;
          p.memi = memi;
          p.depth = depth;
          pieces.piece.push_back(p);
        }
        if (maxdepth < depth) continue;
      }

      newi_list.clear();
      child_time.start();
      {
        for (int i = 0; i <= train.size(); i++)
          dag[i].tiny_node[ind[i]].child.legacy(slow_child[i]);
        vector<int> newi(dags), ci(dags); //current index into child[]
        int fi = 0;
        while (1) {
          next_iteration:
            for (int i = 0; i <= train.size(); i++) {
              auto&child = slow_child[i];//dag[i].node[ind[i]].child;
              while (ci[i] < child.size() &&
                child[ci[i]].first < fi) ci[i]++;
              if (ci[i] == child.size()) goto finish;

              int next_available_fi = child[ci[i]].first;
              if (next_available_fi > fi) {
                fi = next_available_fi;
                goto next_iteration;

              } else {

                newi[i] = child[ci[i]].second;

                if (newi[i] == -1) {
                  fi++;
                  goto next_iteration;
                }
              }
            }
            newi_list.emplace_back(fi, newi);
            fi++;
        }
        finish:;
      }
      child_time.stop();

      for (auto&[fi, newi] : newi_list) {
        if (0) {
          int i = train.size();

          int to = dag[i].tiny_node.getChild(ind[i], fi);
          if (to == TinyChildren::None) {
            string name = dag[i].funcs.getName(fi);
            if (name.substr(0,4) == "Move") {
              newi[i] = dag[i].applyFunc(ind[i], fi);
              if (newi[i] != -1 && out_sizes.size())
              dag[i].applyFunc(newi[i], dag[i].funcs.findfi("embed 1"));
            } else continue;
          } else {
            newi[i] = to; //it->second
          }
          if (newi[i] == -1) continue;
        }

        int new_depth = -1;
        for (int i = 0; i < dags; i++) {
          new_depth = max(new_depth, (int)dag[i].tiny_node[newi[i]].depth);
        }

        int cost = dag[0].funcs.cost[fi];

        if (new_depth >= depth+cost) {
          add(depth+cost, newi);
        }
      }
    }
  }

  piece_time.stop();
  piece_time.print("Piece loop time");
  set_time.print("Set time");
  child_time.print("Child looking time");

  auto lookFor = [&](vector<string> name_list) {
    vector<Image> look_imgs;
    vector<int> look_v;

    int di = 0;
    for (DAG&d : dag) {
      int p = 0;
      for (string name : name_list) {
        int fi = d.funcs.findfi(name);

        int ret = d.tiny_node.getChild(p, fi);
        assert(ret >= 0);

        if (0) {//fi != fi_) {
          cout << p << ' ' << di << " / " << train.size() << endl;
        }

        p = ret;
        assert(p != -1);
      }
      look_v.push_back(p);
      look_imgs.push_back(d.getImg(p));
      di++;
    }
    if (!seen.insert(hashVec(look_v),0).second) cout << "Found indices" << endl;
  };

  if (out_sizes.size() && print_nodes) {
    int nodes = 0;
    for (DAG&d : dag) nodes += d.tiny_node.size();
    cout << "Nodes:  " << nodes << endl;
    cout << "Pieces: " << pieces.piece.size() << endl;
  }
  pieces.dag = move(dag);

  for (Piece3&p : pieces.piece) {
    for (int i = 0; i < pieces.dag.size(); i++) {
      int*ind = &mem[p.memi];
      assert(ind[i] >= 0 && ind[i] < pieces.dag[i].tiny_node.size());
    }
  }

  return pieces;
}
