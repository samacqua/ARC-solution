#include "precompiled_stl.hpp"
using namespace std;
#include "utils.hpp"
#include "brute2.hpp"
#include "visu.hpp"

// =========
// efficient.cpp
// manage search tree
// =========

pair<int,bool> TinyHashMap::insert(ull key, int value) {
  if (table.size() <= data.size()*sparse_factor) {
    table.resize(max((int)(table.size() * resize_step), minsize));
    assert((table.size()&(table.size()-1)) == 0);
    mask = table.size()-1;

    fill(table.begin(), table.end(), -1);
    for (int i = 0; i < data.size(); i++) {
      int&head = table[data[i].key&mask];
      data[i].next = head;
      head = i;
    }
  }
  int&head = table[key&mask];
  int previ = head;
  while (1) {
    if (previ == -1) {
      data.push_back({key, value, head});
      head = data.size()-1;
      return {value, true};
    } else if (data[previ].key == key) {
      return {data[previ].value, false};
    }
    previ = data[previ].next;
  }
}

void TinyChildren::add(int fi, int to) {
  assert(fi >= 0);
  if (size+1 == dense_thres) {
    //Convert to dense
    cap = max(sparse[size-1].first,fi)+1; //Max fi
    pair<int,int>*old = sparse;
    dense = new int[cap];
    fill_n(dense, cap, None);
    dense[fi] = to;
    for (int i = 0; i < size; i++) {
      auto [fi, to] = old[i];
      assert(fi >= 0);
      assert(fi < cap);
      dense[fi] = to;
    }
    assert(old);
    delete[]old;
    size = dense_thres;
  }

  if (size < dense_thres) {
    //Sparse
    if (size+1 > cap) {
      cap = max((cap+1)*3/2,4);
      pair<int,int>*old = sparse;
      sparse = new pair<int,int>[cap];
      copy_n(old, size, sparse);
      if (old) delete[]old;
    }
    {
      int p = size++;
      while (p && sparse[p-1].first > fi) {
	sparse[p] = sparse[p-1];
	p--;
      }
      sparse[p] = {fi, to};
    }
  } else {
    //Dense
    if (cap <= fi) {
      int oldcap = cap;
      int*old = dense;
      cap = (fi+1)*3/2;
      dense = new int[cap];
      fill_n(dense+oldcap, cap-oldcap, None);
      copy_n(old, oldcap, dense);
      assert(old);
      delete[]old;
    }
    dense[fi] = to;
  }
}
int TinyChildren::get(int fi) {
  assert(fi >= 0);
  if (size < dense_thres) {
    int low = 0, high = size-1;
    while (low <= high) {
      int mid = (low+high)>>1;
      int cfi = sparse[mid].first;
      if (cfi == fi) return sparse[mid].second;
      else if (cfi > fi) high = mid-1;
      else low = mid+1;
    }
    return None;
  } else {
    if (fi >= cap) return None;
    return dense[fi];
  }
}
void TinyChildren::legacy(vector<pair<int,int>>&ret) {
  if (size < dense_thres) {
    //Sparse
    ret.resize(size);
    for (int i = 0; i < size; i++)
      ret[i] = sparse[i];
  } else {
    //Dense
    ret.resize(0);
    for (int i = 0; i < cap; i++) {
      if (dense[i] != None)
	ret.emplace_back(i, dense[i]);
    }
  }
}

TinyImage::TinyImage(Image_ img, TinyBank&bank) {
  for (int c : {img.x, img.y, img.w, img.h}) {
    assert(c >= -128 && c < 128);
  }
  x = img.x, y = img.y, w = img.w, h = img.h;

  int freq[10] = {};
  for (char c : img.mask) {
    assert(c >= 0 && c < 10);
    freq[c]++;
  }

  priority_queue<pair<int,int>> pq;
  for (int d = 0; d < 10; d++) {
    if (freq[d]) {
      pq.emplace(-freq[d], -d);
      //cout << d << ": " << freq[d] << endl;
    }
  }
  while (pq.size() < 2) pq.emplace(0,0);

  int nodes = pq.size()-1;
  int pos = nodes-1;
  auto convert = [](int a, int p) {
    if (a <= 0) return -a;
    else {
      assert(9+a-p >= 10 && 9+a-p < 16);
      return 9+a-p;
    }
  };
  while (pq.size() > 1) {
    auto [mfa,a] = pq.top(); pq.pop();
    auto [mfb,b] = pq.top(); pq.pop();
    tree[pos] = convert(a,pos) | convert(b,pos) << 4;
    pq.emplace(mfa+mfb, pos--);
  }
  int code[10] = {}, codelen[10] = {};
  int path[10] = {}, pathlen[10] = {};
  for (int p = 0; p < nodes; p++) {
    for (int k : {0,1}) {
      int child = tree[p]>>k*4&15;
      int newpath = path[p] | k<<pathlen[p];
      if (child < 10) {
	code[child] = newpath;
	codelen[child] = pathlen[p]+1;
      } else {
	child += p-9;
	path[child] = newpath;
	pathlen[child] = pathlen[p]+1;
      }
    }
  }

  assert((bank.curi+align-1)/align < 1ll<<32);
  memi = (bank.curi+align-1)/align;
  size = 0;
  ll memstart = (ll)memi*align;
  bank.alloc();
  for (char c : img.mask) {
    bank.set(memstart+ size, code[c], codelen[c]);
    size += codelen[c];
  }
  bank.curi = memstart+size;
}

Image TinyImage::decompress(TinyBank&bank) {
  Image ret;
  ret.x = x, ret.y = y, ret.w = w, ret.h = h;
  ret.mask.resize(ret.w*ret.h);
  int treep = 0, maski = 0;
  ll memstart = (ll)memi*align;
  for (ll i = memstart; i < memstart+size; i++) {
    int bit = bank.get(i);
    int child = tree[treep]>>bit*4&15;
    if (child < 10) {
      ret.mask[maski++] = child;
      treep = 0;
    } else
      treep += child-9;
  }
  assert(maski == ret.w*ret.h);
  assert(treep == 0);
  return ret;
}

void print_TinyHashMap(TinyHashMap *tiny_hash_map) {
//   struct TinyHashMap {
//   static constexpr double sparse_factor = 1.5, resize_step = 2;
//   static constexpr int minsize = 1<<20;
//   vector<hashEntry> data;
//   vector<int> table;
//   ull mask;
//   };

cout << "sparse_factor: " << tiny_hash_map->sparse_factor;
cout << ", resize_step: " << tiny_hash_map->resize_step;
cout << ", minsize: " << tiny_hash_map->minsize;
cout << ", data: " << "TODO";
// cout << ", table: "; // too large to print
// for (int i=0; i<tiny_hash_map->table.size(); i++) {
//     cout << tiny_hash_map->table[i];
//   }
cout << ", mask: " << tiny_hash_map->mask;
}

void print_TinyNodeBank(TinyNodeBank *tiny_node_bank) {
  // struct TinyNodeBank {
  //   TinyBank bank;
  //   vector<TinyImage> imgs;
  //   vector<TinyNode> node;
  // };

  cout << "bank: ";
  print_TinyBank(&tiny_node_bank->bank);

  cout << ", imgs: ";
  for (int i=0; i<tiny_node_bank->imgs.size(); i++) {
    print_TinyImage(&tiny_node_bank->imgs[i]);
  }

  cout << ", node: ";
  for (int i=0; i<tiny_node_bank->node.size(); i++) {
    print_TinyNode(&tiny_node_bank->node[i]);
  }

}

void print_TinyBank(TinyBank *tiny_bank) {
  // struct TinyBank {
  //   vector<unsigned int> mem;
  //   ll curi = 0;
  // };

  cout << "mem: ";
  for (int i=0; i<tiny_bank->mem.size(); i++) {
    cout << tiny_bank->mem[i] << " | ";
  }
  cout << ", curi: " << tiny_bank->curi;
}

void print_TinyImage(TinyImage *tiny_img) {
  cout << "TODO";
}

void print_TinyNode(TinyNode *tiny_node) {
  cout << "TODO";
}