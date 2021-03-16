#include <map>

struct Sample {
  vector<pair<Image,Image>> train, test;
  Image test_in, test_out;
  string id;    // task ID (ex: "f0afb749")
  int id_ind;   // task ID index (ex: 0)
  FILE*fp;
  Sample(string filename);
  char mygetc();
  int end(char endc);
  void expect(char c);
  string getQuote();
  vector<int> readRow();
  Image readImage();
  vector<Sample> split();
};

vector<Sample> readAll(string path, int maxn);

struct Writer {
  FILE*fp;
  map<string,int> seen;
  Writer(string filename = "submission_part.csv");
  void operator()(const Sample& s, vector<Image> imgs);
  ~Writer();
};

void writeAnswersWithScores(const Sample&s, string filename, vector<Image> imgs, vector<double> scores, vector<string> programs);
