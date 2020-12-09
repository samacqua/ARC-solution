
struct Piece {
  int memi, depth;
};

struct Pieces {
  vector<DAG> dag;
  vector<Piece> piece;
  vector<int> mem;
};

Pieces makePieces(vector<DAG>&dag, vector<pair<Image,Image>> train, vector<point> out_sizes);
