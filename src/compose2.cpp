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

#include "timer.hpp"

#include <map>

// =========
// compose2.cpp
//
// =========

extern int print_times;

struct mybitset
{
  vector<ull> data;
  mybitset(int n)
  {
    data.resize((n + 63) / 64);
  }
  int operator[](int i)
  {
    return data[i >> 6] >> (i & 63) & 1;
  }
  void set(int i, ull v)
  {
    int bit = i & 63;
    data[i >> 6] &= ~(1ull << bit);
    data[i >> 6] |= (v << bit);
  }
  ull hash()
  {
    ull r = 1;
    for (ull h : data)
    {
      r = r * 137139 + h;
    }
    return r;
  }
};

/**
 * number of set bits in x
 * @param x 
 * @returns int
 */
int popcount64d(ull x)
{
  int pop = 0;
  while (x)
  {
    x &= x - 1;
    pop++;
  }
  return pop;
}

vector<Candidate> greedyCompose2(Pieces &pieces, vector<Image> &target, vector<point> out_sizes)
{

  if (pieces.piece.empty())
    return {};
  Timer greedy_fill_time;

  // assert each piece is at a deeper depth than the previous
  {
    int depth = -1;
    for (Piece &p : pieces.piece)
    {
      assert(p.depth >= depth);
      depth = p.depth;
    }
  }

  vector<Image> init;
  vector<int> size;
  {
    for (int i = 0; i < pieces.dag.size(); i++)
    {
      if (i < target.size())
        assert(out_sizes[i] == target[i].size);
      init.push_back(core::full(out_sizes[i], 10));
      size.push_back(init.back().mask.size());
    }
  }

  vector<Candidate> rets;

  int n = pieces.piece.size();

  int M = 0;
  for (int s : size)
    M += s;

  const int M64 = (M + 63) / 64;
  vector<ull> bad_mem, active_mem;
  //vector<mybitset> bad_mem, active_mem;
  vector<int> img_ind, bad_ind, active_ind;

  {
    active_mem.reserve(n * M64);
    bad_mem.reserve(n * M64);
    TinyHashMap seen;
    mybitset badi(M), blacki(M);
    for (int i = 0; i < n; i++)
    {
      int x = 0, y = 0;
      for (int j = 0; j < size.size(); j++)
      {
        int *ind = &pieces.mem[pieces.piece[i].memi];
        Image_ img = pieces.dag[j].getImg(ind[j]);
        const vector<char> &p = img.mask;
        const vector<char> &t = j < target.size() ? target[j].mask : init[j].mask;
        assert(p.size() == size[j]);
        assert(t.size() == size[j]);
        for (int k = 0; k < size[j]; k++)
        {
          badi.set(x++, (p[k] != t[k]));
          blacki.set(y++, (p[k] == 0));
        }
      }
      img_ind.push_back(i);

      active_ind.push_back(active_mem.size());
      for (ull v : blacki.data)
        active_mem.push_back(v);

      bad_ind.push_back(bad_mem.size());
      for (ull v : badi.data)
        bad_mem.push_back(v);
    }
  }

  int max_piece_depth = 0;
  for (int i = 0; i < n; i++)
    max_piece_depth = max(max_piece_depth, pieces.piece[i].depth);

  auto greedyComposeCore = [&](mybitset &cur, const mybitset &careMask, const int piece_depth_thres, vImage &ret) {
    vector<int> sparsej;
    for (int j = 0; j < M64; j++)
    {
      if (~cur.data[j] & careMask.data[j])
        sparsej.push_back(j);
    }

    vector<ull> best_active(M64), tmp_active(M64);
    int besti = -1;
    pair<int, int> bestcnt = {0, 0};
    //for (int i : order) {
    //  if (skip[i] || pieces.piece[img_ind[i]].depth > piece_depth_thres) continue;
    for (int i = 0; i < img_ind.size(); i++)
    {
      if (pieces.piece[img_ind[i]].depth > piece_depth_thres)
        continue;

      for (int k : {0, 1, 2})
      {
        const ull *active_data = &active_mem[active_ind[i]];

        ull flip = (k == 0 ? ~0 : 0);
        ull full = (k == 2 ? ~0 : 0);

        const ull *bad_data = &bad_mem[bad_ind[i]];
        int cnt = 0, covered = 0, ok = 1;
        for (int j = 0; j < M64; j++)
        {
          ull active = ((active_data[j] ^ flip) | full);
          if (~cur.data[j] & bad_data[j] & active)
          {
            ok = 0;
            break;
          }
        }
        if (!ok)
          continue;
        for (int j : sparsej)
        {
          ull active = ((active_data[j] ^ flip) | full);
          cnt += popcount64d(active & ~cur.data[j] & careMask.data[j]);
          //covered += popcount64d(bad.data[j] & cur.data[j] & careMask.data[j]);
        }
        if (ok && make_pair(cnt, -covered) > bestcnt)
        {
          bestcnt = make_pair(cnt, -covered);
          besti = i;

          if (k == 0)
          {
            for (int j = 0; j < M64; j++)
              tmp_active[j] = ~active_data[j];
          }
          else if (k == 1)
          {
            for (int j = 0; j < M64; j++)
              tmp_active[j] = active_data[j];
          }
          else
          {
            for (int j = 0; j < M64; j++)
              tmp_active[j] = ~0;
          }
          best_active = tmp_active;
        }
      }
    }

    if (besti == -1)
      return -1;

    {
      int i = img_ind[besti], x = 0;

      int depth = pieces.piece[i].depth;

      for (int l = 0; l < ret.size(); l++)
      {
        int *ind = &pieces.mem[pieces.piece[i].memi];
        const vector<char> &mask = pieces.dag[l].getImg(ind[l]).mask;
        for (int j = 0; j < size[l]; j++)
        {
          if ((best_active[x >> 6] >> (x & 63) & 1) && ret[l].mask[j] == 10)
          {
            //assert(cur[x-1] == 0);
            ret[l].mask[j] = mask[j];
          }

          x++;
        }
      }
      for (int j = 0; j < M; j++)
      {
        if (best_active[j >> 6] >> (j & 63) & 1)
          cur.set(j, 1);
      }

      return depth;
    }
  };

  map<ull, Image> greedy_fill_mem;

  int maxiters = 10;

  for (int pdt = max_piece_depth % 10; pdt <= max_piece_depth; pdt += 10)
  {
    int piece_depth_thres = pdt;

    for (int it0 = 0; it0 < 10; it0++)
    {
      for (int mask = 1; mask < min(1 << target.size(), 1 << 5); mask++)
      {
        vector<int> maskv;
        for (int j = 0; j < target.size(); j++)
          if (mask >> j & 1)
            maskv.push_back(j);

        int caremask;
        if (it0 < maskv.size())
        {
          caremask = 1 << maskv[it0];
        }
        else
        {
          continue;
        }

        mybitset cur(M), careMask(M);
        {
          int base = 0;
          for (int j = 0; j < size.size(); j++)
          {
            if (!(mask >> j & 1))
              for (int k = 0; k < size[j]; k++)
                cur.set(base + k, 1);
            if ((caremask >> j & 1))
              for (int k = 0; k < size[j]; k++)
                careMask.set(base + k, 1);
            base += size[j];
          }
        }

        int cnt_pieces = 0;
        vector<int> piece_depths;
        int sum_depth = 0, max_depth = 0;

        vector<Image> ret = init;
        for (int it = 0; it < maxiters; it++)
        {

          int depth = greedyComposeCore(cur, careMask, piece_depth_thres, ret);
          if (depth == -1)
            break;
          piece_depths.push_back(depth);
          cnt_pieces++;
          sum_depth += depth;
          max_depth = max(max_depth, depth);

          {
            greedy_fill_time.start();
            vImage cp = ret;
            int carei = 31 - __builtin_clz(caremask);
            assert(caremask == 1 << carei);
            int ok = 1;
            {
              Image &img = cp[carei];
              for (char &c : img.mask)
                if (c == 10)
                  c = 0;
              img = greedyFillBlack(img);
              if (img != target[carei])
                ok = 0;
            }
            if (ok)
            {
              for (int i = 0; i < cp.size(); i++)
              {
                if (i == carei)
                  continue;
                Image &img = cp[i];
                for (char &c : img.mask)
                  if (c == 10)
                    c = 0;
                ull h = hashImage(img);
                if (!greedy_fill_mem.count(h))
                {
                  greedy_fill_mem[h] = greedyFillBlack(img);
                }
                img = greedy_fill_mem[h];
                if (img.w * img.h <= 0)
                  ok = 0;
              }
              if (ok)
                rets.emplace_back(cp, cnt_pieces + 1, sum_depth, max_depth);
            }
            greedy_fill_time.stop();
          }
        }

        /*for (Image&img : ret)
	  for (char&c : img.mask)
	  if (c == 10) c = 0;
	*/

        rets.emplace_back(ret, cnt_pieces, sum_depth, max_depth);
      }
    }
  }

  if (print_times)
    greedy_fill_time.print("Greedy fill time");

  return rets;
}

vector<Candidate> composePieces(Pieces &pieces, vector<pair<Image, Image>> train, vector<point> out_sizes)
{
  vector<Candidate> cands;

  vector<Image> target;
  for (auto [input_grid, output_grid] : train)
    target.push_back(output_grid);

  for (const Candidate &cand : greedyCompose2(pieces, target, out_sizes))
  {
    cands.push_back(cand);
  }
  return cands;
}

vector<Candidate> evaluateCands(const vector<Candidate> &cands, vector<pair<Image, Image>> train)
{
  vector<Candidate> ret;
  for (const Candidate &cand : cands)
  {
    vImage_ imgs = cand.imgs;
    assert(cand.max_depth >= 0 && cand.max_depth < 100);
    assert(cand.cnt_pieces >= 0 && cand.cnt_pieces < 100);
    //cout << cand.max_depth << ' ' << cand.cnt_pieces << endl;
    double prior = cand.max_depth + cand.cnt_pieces * 1e-3; //cnt_pieces;

    int goods = 0;
    for (int i = 0; i < train.size(); i++)
    {
      goods += (imgs[i] == train[i].second);
    }
    double score = goods - prior * 0.01;

    Image answer = imgs.back();
    if (answer.w > 30 || answer.h > 30 || answer.w * answer.h == 0)
      goods = 0;
    for (int i = 0; i < answer.h; i++)
      for (int j = 0; j < answer.w; j++)
        if (answer(i, j) < 0 || answer(i, j) >= 10)
          goods = 0;

    if (goods)
      ret.emplace_back(imgs, score);
  }
  sort(ret.begin(), ret.end());
  //printf("%.20f\n\n", ret[0].score);
  return ret;
}
