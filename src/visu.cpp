#include "precompiled_stl.hpp"
using namespace std;
#include "utils.hpp"
#include "visu.hpp"

// =========
// visu.cpp
// deal with visualization file
// =========

// object to deal with visualization file
Visu::Visu() {
  fp = fopen("visu.txt", "w");
}
Visu::~Visu() {
  fclose(fp);
}
void Visu::next(string s) {
  fprintf(fp, "Task %s\n", s.c_str());
}
void Visu::add(Image in, Image out) {
  fprintf(fp, "Pair\n");
  for (Image_ img : {in,out}) {
    fprintf(fp, "Image %d %d\n", img.w, img.h);
    for (int i = 0; i < img.h; i++) {
      for (int j = 0; j < img.w; j++) {
	int col = img(i,j);
	fprintf(fp, "%d", col);
      }
      fprintf(fp, "\n");
    }
  }
}

// save object/grid
void print(Image img) {
  printf("[%d %d %d %d]\n", img.p.x, img.p.y, img.w, img.h);
  for (int i = 0; i < img.h; i++) {
    for (int j = 0; j < img.w; j++) {
      int col = img(i,j);
      if (col)
	      printf("%d", col);
      else printf(".");
    }
    printf("\n");
  }
}
