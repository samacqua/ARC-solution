struct Visu {
  FILE*fp;
  Visu();
  ~Visu();
  void next(string s);
  void add(Image in, Image out);
};

void print(Image img);
