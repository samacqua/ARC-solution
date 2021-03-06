#include "precompiled_stl.hpp"
using namespace std;

#include "utils.hpp"
#include "core_functions.hpp"

// =========
// core.cpp
// Core functions used for transformations
// =========

namespace core {

  // mask of all colors
  int colMask(Image_ img) {
    int mask = 0;
    for (int i = 0; i < img.h; i++)
      for (int j = 0; j < img.w; j++)
	      mask |= 1<<img(i,j);
    return mask;
  }

  // count number of colors
  int countCols(Image_ img, int include0) {//include0 = 0
    int mask = colMask(img);
    if (!include0) mask = mask&~1;
    return __builtin_popcount(mask);
  }

  // count all non-blank pixels
  int count(Image_ img) {
    int ans = 0;
    for (int i = 0; i < img.h; i++)
      for (int j = 0; j < img.w; j++)
	      ans += img(i,j) > 0;
    return ans;
  }

  // create rectangle of color filling at location p of size size
  Image full(point p, point size, int filling) {//filling = 1
    Image ret;
    ret.p = p;
    ret.size = size;
    ret.mask.assign(ret.h*ret.w, filling);
    return ret;
  }

  // create empty rectangle at location p of size size
  Image empty(point p, point size) {
    return full(p, size, 0);
  }

  // create rectangle of color filling of size size from (0, 0)
  Image full(point size, int filling) {//filling = 1
    Image ret;
    ret.p = {0,0};
    ret.size = size;
    ret.mask.assign(ret.h*ret.w, filling);
    return ret;
  }

  // create empty rectangle of size size
  Image empty(point size) {
    return full(point{0,0}, size, 0);
  }

  // check if image is a rectangle
  bool isRectangle(Image_ a) {
    return count(a) == a.w*a.h;
  }

  // counting what? TODO
  void countComponents_dfs(Image&img, int r, int c) {
    img(r,c) = 0;
    for (int nr = r-1; nr <= r+1; nr++)
      for (int nc = c-1; nc <= c+1; nc++)
	      if (nr >= 0 && nr < img.h && nc >= 0 && nc < img.w && img(nr,nc))
	        countComponents_dfs(img,nr,nc);
  }

  // counting what? number non-blank squares?
  int countComponents(Image img) {
    int ans = 0;
    for (int i = 0; i < img.h; i++) {
      for (int j = 0; j < img.w; j++) {
        if (img(i,j)) {
          countComponents_dfs(img,i,j);
          ans++;
        }
      }
    }
    return ans;
  }

  // get most frequent color
  char majorityCol(Image_ img, int include0) { //include0 = 0
    int cnt[10] = {};
    for (int i = 0; i < img.h; i++)
      for (int j = 0; j < img.w; j++) {
        char c = img(i,j);
        if (c >= 0 && c < 10)
          cnt[c]++;
      }
    if (!include0) cnt[0] = 0;
    int ret = 0;
    int ma = cnt[ret];
    for (int c = 1; c < 10; c++) {
      if (cnt[c] > ma) {
        ma = cnt[c];
        ret = c;
      }
    }
    return ret;
  }

  // mask img starting at p to get a sub-image of size s
  Image subImage(Image_ img, point p, point size) {
    assert(p.x >= 0 && p.y >= 0 && p.x+size.x <= img.w && p.y+size.y <= img.h && size.x >= 0 && size.y >= 0);
    Image ret;
    ret.p = p+img.p;
    ret.size = size;
    ret.mask.resize(ret.w*ret.h);
    for (int i = 0; i < ret.h; i++)
      for (int j = 0; j < ret.w; j++)
	      ret(i,j) = img(i+p.y, j+p.x);
    return ret;
  }

  // split image by color 
  vector<pair<Image,int>> splitCols(Image_ img, int include0) { //include0 = 0
    vector<pair<Image,int>> ret;
    int mask = colMask(img);
    for (char c = !include0; c < 10; c++) {
      if (mask>>c&1) {
	      Image s = img;
        for (int i = 0; i < s.h; i++)
          for (int j = 0; j < s.w; j++)
            s(i,j) = s(i,j) == c;
        ret.emplace_back(s, c);
      }
    }
    return ret;
  }
};
