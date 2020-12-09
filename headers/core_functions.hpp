namespace core {
  int colMask(Image_ img);
  int countCols(Image_ img, int include0 = 0);
  int count(Image_ img);
  Image full(point p, point size, int filling = 1);
  Image empty(point p, point size);
  Image full(point size, int filling = 1);
  Image empty(point size);
  bool isRectangle(Image_ a);
  int countComponents(Image img);
  char majorityCol(Image_ img, int include0 = 0);
  Image subImage(Image_ img, point p, point size);
  vector<pair<Image,int>> splitCols(Image_ img, int include0 = 0);
};
