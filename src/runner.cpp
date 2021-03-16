#include "precompiled_stl.hpp"

using namespace std;

#include "utils.hpp"
#include "core_functions.hpp"
#include "image_functions.hpp"
#include "visu.hpp"
#include "read.hpp"
#include "normalize.hpp"
#include "tasks.hpp"
#include "evals.hpp"
#include "brute2.hpp"
#include "score.hpp"
#include "load.hpp"
#include "deduce_op.hpp"
#include "pieces.hpp"
#include "compose2.hpp"
#include "brute_size.hpp"

#include <thread>

// =========
// runner.cpp
// puts together all logic to search in parallel
// =========

// format string so cout is colored
string green(string s) {
  return ("\033[1;32m"+s+"\033[0m");
}
string blue(string s) {
  return ("\033[1;34m"+s+"\033[0m");
}
string yellow(string s) {
  return ("\033[1;33m"+s+"\033[0m");
}
string red(string s) {
  return ("\033[1;31m"+s+"\033[0m");
}

// print if got task correct
void writeVerdict(int si, string sid, int verdict) {
  printf("Task #%2d (%s): ", si, sid.c_str());
  switch (verdict) {
  case 3: cout << green("Correct") << endl; break;
  case 2: cout << yellow("Candidate") << endl; break;
  case 1: cout << blue("Dimensions") << endl; break;
  case 0: cout << red("Nothing") << endl; break;
  default: assert(0);
  }
}

int MAXDEPTH = -1; //Argument
int MAXSIDE = 100, MAXAREA = 40*40, MAXPIXELS = 40*40*5; //Just default values
int print_times = 1, print_mem = 1, print_nodes = 1;

void run(int single_task_id = -1, int arg = -1) {

  // parse args
  int no_norm   = (arg >= 10 && arg < 20);
  int add_flips = (arg >= 20 && arg < 40);
  int add_flip_id = (arg >= 30 && arg < 40 ? 7 : 6);

  // arg is max depth i guess?
  if (arg == -1) arg = 2;
  MAXDEPTH = arg % 10 * 10;

  int eval = 0;
  int skips = 0;

  // if training == true, set inds = range(0,416) in summary.py
  // if training == false, set inds = range(0,419) in summary.py
  bool training = false;
  string sample_dir = (training) ? "training" : "evaluation";

  int samples = -1;
  if (eval) {
    sample_dir = "test";
  }

  vector<Sample> sample = readAll(sample_dir, samples);

  int scores[4] = {};
  Visu visu;
  vector<int> verdict(sample.size());
  int dones = 0;
  Loader load(sample.size());

  assert(single_task_id < sample.size());
  //Remember to fix Timers before running parallel

  for (int si = 0; si < sample.size(); si++) {
    if (single_task_id != -1 && si != single_task_id) continue;

    //if (si == 30) assert(0);

    if (eval) load();
    else if (++dones % 10 == 0) {
      cout << dones << " / " << sample.size() << endl;
    }

    const Sample&s = sample[si];

    //Normalize sample
    // this recolors the sample -- if visualizing set no_norm to false
    Simplifier sim = normalizeCols(s.train);
    if (no_norm) sim = normalizeDummy(s.train);

    // get training pairs
    vector<pair<Image,Image>> train;
    for (auto&[in,out] : s.train) {
      train.push_back(sim(in,out));
    }

    // for each training pair, add the pair flipped along diagonals
    if (add_flips) {
      for (auto&[in,out] : s.train) {
        auto [rin,rout] = sim(in,out);
        train.push_back({rigid(rin,add_flip_id),rigid(rout,add_flip_id)});
      }
    }
    auto [test_in,test_out] = sim(s.test_in, s.test_out);

    {
      int insumsize = 0, outsumsize = 0, maxcols = 0;
      int maxside = 0, maxarea = 0;
      for (auto&[in,out] : s.train) {
        maxside = max({maxside, in.w, in.h, out.w, out.h});
        maxarea = max({maxarea, in.w*in.h, out.w*out.h});
        insumsize += in.w*in.h;
        outsumsize += out.w*out.h;
        maxcols = max(maxcols, __builtin_popcount(core::colMask(in)));
      }
      int sumsize = max(insumsize, outsumsize);

      cerr << "Features: " << insumsize << ' ' << outsumsize << ' ' << maxcols << endl;
      cerr << "MAXDEPTH: " << MAXDEPTH << endl;

      MAXSIDE = 100;
      MAXAREA = maxarea*2;
      MAXPIXELS = MAXAREA*5;
    }

    // get all predicted output sizes
    vector<point> out_sizes = bruteSize(test_in, train);

    // generate candidate pieces
    Pieces pieces;
    {
      double start_time = now();
      vector<DAG> dags = brutePieces(test_in, train, out_sizes);

      if (print_times) cout << "brutePieces time: " << now()-start_time << endl;
      start_time = now();
      pieces = makePieces(dags, train, out_sizes);
      if (print_times) cout << "makePieces time: " << now()-start_time << endl;
    }

    // Print information about memory usage
    if (print_mem) {
      double size = 0, child = 0, other = 0, inds = 0, maps = 0;
      for (DAG&d : pieces.dag) {
        other += sizeof(TinyNode)*d.tiny_node.size();
        size += 4*d.tiny_node.bank.mem.size();
        for (TinyNode&n : d.tiny_node.node) {
          if (n.child.size < TinyChildren::dense_thres)
            child += n.child.cap*8;
          else
            child += n.child.cap*4;
        }
	      maps += 16*d.hashi.data.size()+4*d.hashi.table.size();
      }
      for (Piece&p : pieces.piece) {
	      inds += sizeof(p);
      }
      inds += sizeof(pieces.mem[0])*pieces.mem.size();
      printf("Memory: %.1f + %.1f + %.1f + %.1f + %.1f MB\n", size/1e6, child/1e6, other/1e6, maps/1e6, inds/1e6);
    }

    for (DAG&d : pieces.dag) {
      d.hashi.clear();
      for (TinyNode&n : d.tiny_node.node) {
	      n.child.clear();
      }
    }

    int s1 = 0;
    if (!eval) s1 = (out_sizes.back() == test_out.size);

    //Assemble pieces into candidates
    vector<Candidate> cands;
    {
      double start_time = now();
      cands = composePieces(pieces, train, out_sizes);
      if (print_times) cout << "composePieces time: " << now()-start_time << endl;
    }
    addDeduceOuterProduct(pieces, train, cands);

    cands = evaluateCands(cands, train);

    int s2 = 0;
    if (!eval) s2 = scoreCands(cands, test_in, test_out);

    //Pick best candidates
    vector<Candidate> answers = cands;

    {
      sort(cands.begin(), cands.end());
      vector<Candidate> filtered;
      set<ull> seen;
      for (const Candidate&cand : cands) {
        ull h = hashImage(cand.imgs.back());
        if (seen.insert(h).second) {
          filtered.push_back(cand);
          if (filtered.size() == 3+skips*3) break;
        }
      }
      for (int i = 0; i < skips*3 && filtered.size(); i++)
        filtered.erase(filtered.begin());
      answers = move(filtered);
    }

    //Reconstruct answers
    vector<Image> rec_answers;
    vector<double> answer_scores;
    for (Candidate&cand : answers) {
      rec_answers.push_back(sim.rec(s.test_in, cand.imgs.back()));
      double score = cand.score;
      if (add_flips) {
	      score /= 2-1e-5;
      }
      answer_scores.push_back(score);
    }

    int s3 = 0;
    if (!eval) s3 = scoreAnswers(rec_answers, s.test_in, s.test_out);

    // add info to visualizer
    if (!eval) {//!eval && s1 && !s2) {
      {
        visu.next(to_string(si) + " - test");
        for (auto&[in,out] : train) visu.add(in,out);
        visu.next(to_string(si) + " - test");
        visu.add(test_in,test_out);
        visu.next(to_string(si) + " - cands");
        for (int i = 0; i < min((int)answers.size(), 5); i++) {
          visu.add(test_in, answers[i].imgs.back());
        }
      }
    }

    // add info about success
    if (!eval) {
      if (s3) verdict[si] = 3;      // top 3 submission
      else if (s2) verdict[si] = 2; // generated correct output, not included in top 3
      else if (s1) verdict[si] = 1; // generated correct grid size
      else verdict[si] = 0;         // nothing correct
      scores[verdict[si]]++;

      writeVerdict(si, s.id, verdict[si]);
    }
    {
      string fn = "output/answer_"+to_string(single_task_id)+"_"+to_string(arg)+".csv";
      writeAnswersWithScores(s, fn, rec_answers, answer_scores);
    }
  }

  // for all submissions, write if correct and then summarize
  if (!eval && single_task_id == -1) {
    for (int si = 0; si < sample.size(); si++) {
      Sample&s = sample[si];
      writeVerdict(si, s.id, verdict[si]);
    }
    for (int si = 0; si < sample.size(); si++) cout << verdict[si];
    cout << endl;

    for (int i = 3; i; i--) scores[i-1] += scores[i];
    printf("\n");
    printf("Total: % 4d\n", scores[0]);
    printf("Pieces:% 4d\n", scores[1]);
    printf("Cands: % 4d\n", scores[2]);
    printf("Correct:% 3d\n", scores[3]);
  }
}
