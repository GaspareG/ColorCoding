/*
  Author: Gaspare Ferraro
  Count and find simple k colorful-path in a graph
  using the color-coding technique (parallel version)
*/
#include <vector>
#include <set>
#include <map>
#include <string>
#include <algorithm>
#include <iterator>
#include <random>
#include <limits>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <omp.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#ifdef K_8
#define MAXK 8
#define COLORSET uint8_t
#elif K_16
#define MAXK 16
#define COLORSET uint16_t
#elif K_64
#define MAXK 64
#define COLORSET uint64_t
#else
#define MAXK 32
#define COLORSET uint32_t
#endif

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || \
    (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/resource.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>

#elif(defined(_AIX) || defined(__TOS__AIX__)) || \
    (defined(__sun__) || defined(__sun) ||       \
     defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || \
    defined(__gnu_linux__)
#include <stdio.h>

size_t getCurrentRSS() {
#if defined(_WIN32)
  /* Windows -------------------------------------------------- */
  PROCESS_MEMORY_COUNTERS info;
  GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
  return (size_t)info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
  /* OSX ------------------------------------------------------ */
  struct mach_task_basic_info info;
  mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info,
                &infoCount) != KERN_SUCCESS)
    return (size_t)0L; /* Can't access? */
  return (size_t)info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || \
    defined(__gnu_linux__)
  /* Linux ---------------------------------------------------- */
  long rss = 0L;
  FILE *fp = NULL;
  if ((fp = fopen("/proc/self/statm", "r")) == NULL)
    return (size_t)0L; /* Can't open? */
  if (fscanf(fp, "%*s%ld", &rss) != 1) {
    fclose(fp);
    return (size_t)0L; /* Can't read? */
  }
  fclose(fp);
  return (size_t)rss * (size_t)sysconf(_SC_PAGESIZE);

#else
  /* AIX, BSD, Solaris, and Unknown OS ------------------------ */
  return (size_t)0L; /* Unsupported. */
#endif
}

#endif

#else
#error "Cannot define getPeakRSS( ) or getCurrentRSS( ) for an unknown OS."
#endif

using namespace std;
typedef long long ll;

unsigned int N, M;
unsigned q = 0;
unsigned thread_count = 0;
static int verbose_flag, help_flag;

ll cont = 0;
int *color;
char *label;
vector<int> *G;
int Sa, Sb;
int *A, *B;

inline int nextInt() {
  int r;
  scanf("%d", &r);
  return r;
}

// Random generator
random_device rd;
mt19937_64 eng = mt19937_64(rd());
uniform_int_distribution<unsigned long long> distr;

// Get pos-th bit in n
bool getBit(COLORSET n, int pos) { return ((n >> pos) & 1) == 1; }

// Set pos-th bit in n
COLORSET setBit(COLORSET n, int pos) { return n |= 1 << pos; }

// Reset pos-th bit in n
COLORSET clearBit(COLORSET n, int pos) { return n &= ~(1 << pos); }

// Complementary set of a COLORSET
COLORSET getCompl(COLORSET n) { return ((1 << q) - 1) & (~n); }

// Random coloring graph using k color
inline void randomColor() {
  for (unsigned int i = 0; i < N; i++) color[i] = eng() % q;
}

// Path label
string L(vector<int> P) {
  string l = "";
  for (size_t i = 0; i < P.size(); i++) l += label[P[i]];
  return l;
}

// // Link
// map<pair<int, COLORSET>, vector<int>> links;
//
// inline void addLink(int x, COLORSET C, int j) {
//   auto key = make_pair(x, C);
//   if (links.find(key) == links.end()) links[key] = vector<int>();
//   links[key].push_back(j);
// }
//
// // Oracle
// vector<int> H(int x, COLORSET C) { return links[make_pair(x, C)]; }
//
// void list_k_path(vector<int> ps, COLORSET cs, int x) {
//   vector<int> oracle = H(x, cs);
//   if (ps.size() + 1 == q) {
//     cont++;
//     for (int j : ps) printf("[%6d] ", j);
//     printf("\n");
//     for (int j : ps) printf("[%6d] ", color[j]);
//     printf("\n");
//   } else
//     for (int v : oracle) {
//       ps.push_back(v);
//       list_k_path(ps, setBit(cs, color[v]), v);
//       ps.pop_back();
//     }
// }

// bruteforce
set<string> dict;
map<pair<int, string>, ll> freqBrute;

vector<int> P[30];
string Pstring[30];
set<int> Pset[30];

void dfs(int t, int u, int k) {
  if (Pset[t].find(u) != Pset[t].end()) return;

  Pset[t].insert(u);
  Pstring[t].push_back(label[u]);
  P[t].push_back(u);// // Link
// map<pair<int, COLORSET>, vector<int>> links;
//
// inline void addLink(int x, COLORSET C, int j) {
//   auto key = make_pair(x, C);
//   if (links.find(key) == links.end()) links[key] = vector<int>();
//   links[key].push_back(j);
// }
//
// // Oracle
// vector<int> H(int x, COLORSET C) { return links[make_pair(x, C)]; }
//
// void list_k_path(vector<int> ps, COLORSET cs, int x) {
//   vector<int> oracle = H(x, cs);
//   if (ps.size() + 1 == q) {
//     cont++;
//     for (int j : ps) printf("[%6d] ", j);
//     printf("\n");
//     for (int j : ps) printf("[%6d] ", color[j]);
//     printf("\n");
//   } else
//     for (int v : oracle) {
//       ps.push_back(v);
//       list_k_path(ps, setBit(cs, color[v]), v);
//       ps.pop_back();
//     }
// }

  if (k == 0) {
    #pragma omp critical
    {
      dict.insert(Pstring[t]);
      freqBrute[make_pair(*P[t].begin(), Pstring[t])]++;
    }
  } else
    for (int v : G[u]) dfs(t, v, k - 1);

  Pset[t].erase(u);
  Pstring[t].pop_back();
  P[t].pop_back();
}

map<COLORSET, ll> *DP[MAXK + 1];

void processDP() {
  if (verbose_flag) printf("K = %u\n", 1);
  for (unsigned int u = 0; u < N; u++) DP[1][u][setBit(0, color[u])] = 1ll;
  for (unsigned int i = 2; i <= q; i++) {
    if (verbose_flag) printf("K = %u\n", i);
    #pragma omp parallel for schedule(static, 1)
    for (unsigned int u = 0; u < N; u++) {
      for (int v : G[u]) {
        for (auto d : DP[i - 1][v]) {
          COLORSET s = d.first;
          ll f = d.second;
          if (getBit(s, color[u])) continue;
          ll fp = DP[i][u][setBit(s, color[u])];
          DP[i][u][setBit(s, color[u])] = f + fp;
        }
      }
    }
  }
}

bool isPrefix(set<string> W, string x) {
  auto it = W.lower_bound(x);
  if (it == W.end()) return false;
  return mismatch(x.begin(), x.end(), (*it).begin()).first == x.end();
}

map<string, ll> processFrequency(set<string> W, multiset<int> X) {
  set<string> WR;
  for (string w : W) {
    reverse(w.begin(), w.end());
    WR.insert(w);
  }
  vector<tuple<int, string, COLORSET>> old;

  for (int x : X)
    if (isPrefix(WR, string(&label[x], 1)))
      old.push_back(make_tuple(x, string(&label[x], 1), setBit(0ll, color[x])));

  for (int i = q - 1; i > 0; i--) {
    vector<tuple<int, string, COLORSET>> current;
    #pragma omp parallel for schedule(static, 1)
    for (int j = 0; j < (int)old.size(); j++) {
      auto o = old[j];
      int u = get<0>(o);
      string LP = get<1>(o);
      COLORSET CP = get<2>(o);
      for (int v : G[u]) {
        if (getBit(CP, color[v])) continue;
        COLORSET CPv = setBit(CP, color[v]);
        string LPv = LP + label[v];
        if (!isPrefix(WR, LPv)) continue;
        #pragma omp critical
        { current.push_back(make_tuple(v, LPv, CPv)); }
      }
    }
    old = current;
  }
  map<string, ll> frequency;
  for (auto c : old) {
    string s = get<1>(c);
    reverse(s.begin(), s.end());
    frequency[s]++;
  }
  return frequency;
}

vector<int> randomPathTo(int u) {
  vector<int> P;
  P.push_back(u);
  COLORSET D = getCompl(setBit(0l, color[u]));
  for (int i = q - 1; i > 0; i--) {
    vector<ll> freq;
    for (int v : G[u]) freq.push_back(DP[i][v][D]);
    discrete_distribution<int> distribution(freq.begin(), freq.end());
    u = G[u][distribution(eng)];
    P.push_back(u);
    D = clearBit(D, color[u]);
  }
  reverse(P.begin(), P.end());
  return P;
}

set<string> randomColorfulSample(vector<int> X, int r) {
  set<string> W;
  set<vector<int>> R;
  vector<ll> freqX;
  for (int x : X) freqX.push_back(DP[q][x][getCompl(0ll)]);
  discrete_distribution<int> distribution(freqX.begin(), freqX.end());
  while (R.size() < (size_t)r) {
    int u = X[distribution(eng)];
    vector<int> P = randomPathTo(u);
    if (R.find(P) == R.end()) R.insert(P);
  }
  for (auto r : R) {
    reverse(r.begin(), r.end());
    W.insert(L(r));
  }
  return W;
}

map<pair<int, string>, ll> randomColorfulSamplePlus(vector<int> X, int r) {
  map<pair<int, string>, ll> W;
  set<vector<int>> R;
  vector<ll> freqX;
  for (int x : X) freqX.push_back(DP[q][x][getCompl(0ll)]);
  discrete_distribution<int> distribution(freqX.begin(), freqX.end());
  while (R.size() < (size_t)r) {
    int u = X[distribution(eng)];
    vector<int> P = randomPathTo(u);
    if (R.find(P) == R.end()) R.insert(P);
  }
  for (auto r : R) {
    reverse(r.begin(), r.end());
    W[make_pair(*r.begin(), L(r))]++;
  }
  return W;
}

set<string> BCSampler(set<int> A, set<int> B, int r) {
  vector<int> X;
  for (int a : A) X.push_back(a);
  for (int b : B) X.push_back(b);
  return randomColorfulSample(X, r);
}

vector<int> naiveRandomPathTo(int u) {
  vector<int> P;
  set<int> Ps;
  P.push_back(u);
  Ps.insert(u);
  for (int i = q - 1; i > 0; i--) {
    vector<int> Nu;
    for (int j : G[u])
      if (Ps.find(j) == Ps.end()) Nu.push_back(j);
    if (Nu.size() == 0) return P;
    int u = Nu[rand() % Nu.size()];
    Ps.insert(u);
    P.push_back(u);
  }
  return P;
}

map<pair<int, string>, ll> baselineSampler(vector<int> X, int r) {
  set<vector<int>> R;
  while (R.size() < (size_t)r) {
    int u = X[rand() % X.size()];
    vector<int> P = naiveRandomPathTo(u);
    if (P.size() == q && R.find(P) == R.end()) R.insert(P);
  }
  map<pair<int, string>, ll> fx;
  for (auto P : R) fx[make_pair(*P.begin(), L(P))]++;
  return fx;
}

double FJW(set<string> W, set<int> A, set<int> B) {
  multiset<int> AiB, AB;
  for (int a : A) AB.insert(a);
  for (int b : B)
    if (AB.find(b) == AB.end()) AB.insert(b);
  for (int a : A)
    if (B.find(a) != B.end()) AiB.insert(a);
  long long num = 0ll;
  long long den = 0ll;
  map<string, ll> freqAiB = processFrequency(W, AiB);
  map<string, ll> freqAB = processFrequency(W, AB);
  for (string w : W) {
    num += freqAiB[w];
    den += freqAB[w];
  }
  return (double)num / (double)den;
}

double BCW(set<string> W, map<string, ll> freqA, map<string, ll> freqB,
           long long R) {
  ll num = 0ll;
  for (string x : W) {
    ll fax = freqA[x];
    ll fbx = freqB[x];
    num += 2 * min(fax, fbx);
  }
  return (double)num / (double)R;
}

double BCW(set<string> W, map<string, ll> freqA, map<string, ll> freqB) {
  ll num = 0ll;
  ll den = 0ll;
  for (string x : W) {
    ll fax = freqA[x];
    ll fbx = freqB[x];
    num += 2 * min(fax, fbx);
    den += fax + fbx;
  }
  return (double)num / (double)den;
}

double FJW(set<string> W, map<string, ll> freqA, map<string, ll> freqB, long long R) {
  ll num = 0ll;
  for (string x : W) {
    ll fax = freqA[x];
    ll fbx = freqB[x];
    num += min(fax, fbx);
  }
  return (double)num / (double)R;
}

double BCW(set<string> W, set<int> A, set<int> B) {
  ll num = 0ll;
  ll den = 0ll;
  multiset<int> mA, mB;
  for (int a : A) mA.insert(a);
  for (int b : B) mB.insert(b);
  map<string, ll> freqA = processFrequency(W, mA);
  map<string, ll> freqB = processFrequency(W, mB);
  vector<string> vW = vector<string>(W.begin(), W.end());
  for (int i = 0; i < (int)vW.size(); i++) {
    string w = vW[i];
    long long fax = freqA[w];
    long long fbx = freqB[w];
    num += 2 * min(fax, fbx);
    den += fax + fbx;
  }
  return (double)num / (double)den;
}

void print_usage(char *filename) {
  printf(
      "Usage: ./%s -q length -g filename -p threadcount -s filename"
      "--help --verbose\n",
      filename);
  printf("Valid arguments:\n");

  printf("-q, --path length\n");
  printf("\tLength of the path.\n");

  printf("-g, --input filename\n");
  printf("\tInput file of labeled graph in nmle format (default stdin)\n");

  printf("-p, --parallel threadcount\n");
  printf("\tNumber of threads to use (default maximum thread avaiable)\n");

  printf("--help\n");
  printf("\tDisplay help text and exit.\n");

  printf("--verbose\n");
  printf("\nPrint status messages.\n");
}

bool input_graph_flag = false;
char *input_graph = NULL;

long long current_timestamp() {
  struct timeval te;
  gettimeofday(&te, NULL);  // get current time
  long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000;
  return milliseconds;
}

int main(int argc, char **argv) {
  static struct option long_options[] = {
      {"path", required_argument, 0, 'q'},
      {"input", required_argument, 0, 'g'},
      {"parallel", required_argument, 0, 'p'},
      {"help", no_argument, &help_flag, 1},
      {"verbose", no_argument, &verbose_flag, 1},
      {0, 0, 0, 0}};

  int option_index = 0;
  int c;
  while (1) {
    c = getopt_long(argc, argv, "q:g:p:", long_options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 'q':
        if (optarg != NULL) q = atoi(optarg);
        break;
      case 'g':
        input_graph_flag = true;
        if (optarg != NULL) input_graph = optarg;
        break;
      case 'p':
        if (optarg != NULL) thread_count = atoi(optarg);
        break;
    }
  }

  if (help_flag || argc == 1) {
    print_usage(argv[0]);
    return 0;
  }

  if (q == 0) {
    printf("Invalid or missing path length value.\n");
    return 1;
  }

  if (q > MAXK) {
    printf("q to high! (max value: %d\n", MAXK);
    return 1;
  }

  if (thread_count > 0 && (int)thread_count < omp_get_max_threads()) {
    omp_set_dynamic(0);
    omp_set_num_threads(thread_count);
  }

  if (verbose_flag) {
    printf("Options:\n");
    printf("Q = %d\n", q);
    printf("thread = %d\n", thread_count);
    printf("input_graph = %s\n", input_graph != NULL ? input_graph : "stdin");
  }

  if (verbose_flag) printf("Reading graph...\n");

  if (input_graph_flag) {
    if (input_graph == NULL) {
      printf("Input file name missing!\n");
      return 1;
    }

    int input_fd = open(input_graph, O_RDONLY, 0);
    if (input_fd == -1) {
      perror("Error opening input file");
      return 1;
    }
    read(input_fd, &N, sizeof(int));
    read(input_fd, &M, sizeof(int));
    if (verbose_flag) printf("N = %d | M = %d\n", N, M);

    label = new char[N + 1];
    color = new int[N + 1];
    int *intLabel = new int[N + 1];

    if (verbose_flag) printf("Reading labels...\n");
    read(input_fd, intLabel, N * sizeof(int));
    for (unsigned int i = 0; i < N; i++) label[i] = 'A' + intLabel[i];

    if (verbose_flag) printf("Reading edges...\n");
    G = new vector<int>[N + 1];
    int *ab = new int[2 * M];
    read(input_fd, ab, 2 * M * sizeof(int));
    for (unsigned int i = 0; i < M; i++) {
      G[ab[2 * i]].push_back(ab[2 * i + 1]);
      G[ab[2 * i + 1]].push_back(ab[2 * i]);
    }
    free(ab);

  } else {
    // Read from stdin, nme format
    N = nextInt();
    M = nextInt();
    if (verbose_flag) printf("N = %d | M = %d\n", N, M);

    label = new char[N + 1];
    color = new int[N + 1];
    G = new vector<int>[N + 1];

    if (verbose_flag) printf("Reading labels...\n");
    for (unsigned int i = 0; i < N; i++) label[i] = 'A' + nextInt();

    if (verbose_flag) printf("Reading edges...\n");
    for (unsigned int i = 0; i < M; i++) {
      int a = nextInt();
      int b = nextInt();
      G[a].push_back(b);
      G[b].push_back(a);
    }
  }

  if (verbose_flag) printf("N = %d | M = %d\n", N, M);

  if (verbose_flag) printf("|A| = %d | |B| = %d\n", Sa, Sb);

  // Create DP Table
  for (unsigned int i = 0; i <= q + 1; i++)
    DP[i] = new map<COLORSET, ll>[N + 1];

  // Random color graph
  if (verbose_flag) printf("Random coloring graph...\n");
  randomColor();

  // Add fake node N connected to all the nodes
  for (unsigned int i = 0; i < N; i++) {
    G[N].push_back(i);
    G[i].push_back(N);
  }
  color[N] = q;
  N++;
  q++;

  // Fill dynamic programming table
  if (verbose_flag) printf("Processing DP table...\n");
  ll time_a = current_timestamp();
  processDP();
  ll time_b = current_timestamp() - time_a;
  if (verbose_flag) printf("End processing DP table [%llu]ms\n", time_b);

  ll time_dp = time_b;
  // list_k_path(vector<int>(), setBit(0ll, color[N-1]), N-1);
  N--;
  q--;
  for (unsigned int i = 0; i < N; i++) G[i].pop_back();

  vector<int> sampleV;
  for (unsigned int i = 0; i < N; i++) sampleV.push_back(i);

  // vector<pair<int, int>> ABsize;
  //  ABsize.push_back(make_pair(4,4));
  // ABsize.push_back(make_pair(1, 1));
  // ABsize.push_back(make_pair(500, 500));
  // ABsize.push_back(make_pair(1000, 1000));

  // ABsize.push_back(make_pair(10, 10));
  // ABsize.push_back(make_pair(100, 100));
  // ABsize.push_back(make_pair(1, 10));
  // ABsize.push_back(make_pair(1, 100));

  vector<double> epsilonV;
  epsilonV.push_back(0.2);

  dict.clear();
  freqBrute.clear();

  double bc_brute;  // BC-BRUTE
  // double bc_alg3;   // BC-2PLUS
  double bc_2plus;  // BC-2PLUS
  double bc_base;   // BC-2PLUS

  // double bc_alg3_rel;   // BC-2PLUS_REL
  // double bc_2plus_rel;  // BC-2PLUS_REL
  // double bc_base_rel;   // BC-2PLUS_REL

  double fj_brute;  // FJ-BRUTE_REL
  // double fj_alg3;   // FJ-2PLUS
  double fj_2plus;  // FJ-2PLUS
  double fj_base;   // FJ-2PLUS

  // double fj_2plus_rel;  // FJ-2PLUS_REL
  // double fj_alg3_rel;   // FJ-2PLUS_REL
  // double fj_base_rel;   // FJ-2PLUS_REL

  int tau_brute;  // TAU
  // int tau_alg3;   // TAU
  int tau_base;   // TAU
  int tau_2plus;  // TAU

  size_t vmrss_brute = 0;  // VmRSS
  // size_t vmrss_alg3 = 0;   // VmRSS
  size_t vmrss_2plus = 0;  // VmRSS
  size_t vmrss_base = 0;   // VmRSS

  ll time_brute = 0ll;  // TIME
  // ll time_alg3 = 0ll;   // TIME
  ll time_2plus = 0ll;  // TIME
  ll time_base = 0ll;   // TIME

  srand(42);

  int pacino[] = { 255990,452410,452417,791850,706098,452428,555162,360012,452441,2303,403666,556256,15073,15074,689303,576706,15096,54702,160270,22699,669510,706106,70329,10606,15214,15215,25531,103800,229893,15281,68757,44172,15302,456843,28287,79651,562522,435593,50887,15357,54850,28354,31803,54858,22961,118459,31814,133366,25797,15463,28476,15470,23091,438821,54968,118555,20438,386429,438831,127251,127253,15571,438971,152780,438973,55032,49453,15621,128413,15645,69022,349740,23325,15662,15665,15695,55126,136352,70895,55207,47295,57339,194595,118841,130600,194631,194630,71026,80050,15937,114322,194650,31855,15976,194597,186229,16017,12688,80130,141241,194616,23923,149725,16100,194619,133713,16116,194627,117859,88299,191119,55417,55421,194645,145653,63160,16287,80268,57707,12681,55487,132988,16324,16325,24193,194607,194610,117940,55523,16376,194617,19613,194618,55552,30805,29703,136103,88513,42485,104548,30841,166435,194636,138109,24363,16485,111317,194640,16513,46902,194652,77706,29820,190385,80420,31856,100152,55678,16566,47456,194601,119277,27283,16620,118122,159733,21918,6123,44447,16679,185743,119395,194635,16725,16794,60001,55907,58161,47206,53488,16858,22186,194611,194613,141817,119487,160534,194621,111141,16949,165334,22330,30424,17003,119560,56057,80773,194661 };

  int deniro[] = {860707,359359,711946,452408,423268,335053,466088,31729,555162,360012,15019,43955,15023,403666,470145,425804,27975,706103,54636,639102,706721,15070,15073,15074,214602,467500,54350,363635,706575,522376,2397,550376,49007,15146,15151,15166,390677,536968,274185,343546,54726,44092,360677,68682,669357,557641,453886,15219,20070,44129,25531,208888,378725,77591,103794,80884,229893,15278,228401,35347,312240,334245,328550,44206,101866,56809,28354,15374,31803,22961,79690,438800,15397,233511,23008,438805,15420,274944,103921,25797,20322,10805,74234,54912,78488,77714,15463,28476,352094,58739,117419,23091,70614,132630,10870,438822,279506,70649,386429,98617,58787,54983,15529,47488,438968,15540,15548,127253,296029,15571,256211,438971,118618,15589,42473,55032,49453,15621,58889,126971,386446,26059,438843,381784,15642,57102,81151,438846,23325,15659,15662,70794,438850,15684,44494,256237,296592,15695,310528,349150,26154,117568,378870,347054,20691,12617,81211,79889,315551,73639,63158,378879,15742,31718,272167,237287,93119,160498,15788,23510,28882,117635,160512,20835,117643,275108,349777,31765,70958,77645,438867,15850,81318,160539,11147,31785,42479,327851,178768,71026,42985,20966,81370,15937,129557,127482,42532,55297,245121,349712,237357,438999,16002,16004,118933,69342,81439,16017,43940,12688,80130,16035,191713,42431,438882,43960,303232,439009,23899,98324,138944,16080,16090,55360,119003,78158,132913,29334,289558,438889,204040,16144,400198,117858,117859,55417,439027,188341,46641,3514,438895,317758,119063,24062,188345,55455,188275,16281,16287,57707,346981,12681,16306,84094,16314,119112,24193,133830,26967,209630,201364,16386,99394,80330,117965,42466,44217,119164,27019,31808,52656,438913,16449,30841,126988,208927,55599,40797,136121,119203,111317,114469,16513,55630,151089,226139,204009,350574,188343,27140,80412,44311,27147,81730,31856,63007,16547,12767,16554,352087,46950,16566,55690,237176,81762,16577,55703,77497,120153,119283,57966,71645,352100,27283,55735,81787,80483,313792,16620,118122,352110,352111,44429,44855,188323,71716,6123,44447,31815,131290,16695,100931,330648,160442,349745,349746,144803,55856,24731,252096,16761,16762,16783,127042,114596,16794,352127,63159,55887,42397,352129,119429,16812,77590,179061,16829,221279,150058,16833,16845,119457,16858,127398,118294,16875,343523,16876,250117,44618,27689,118305,240201,19633,16913,330041,16949,16954,49732,22330,160573,107963,115346,80748,17003,70117,119560,127760,352150,119578,17030,101736,80787};

  set<int> A = set<int>(pacino, pacino+198);

  set<int> B = set<int>(deniro, deniro+382);

  for(double epsilon : epsilonV)
  {
    // random_shuffle(sampleV.begin(), sampleV.end());
    // set<int> A = set<int>(sampleV.begin(), sampleV.begin() + ABs.first);
    // random_shuffle(sampleV.begin(), sampleV.end());
    // set<int> B = set<int>(sampleV.begin(), sampleV.begin() + ABs.second);
    vector<int> X;
    for (int a : A) X.push_back(a);
    for (int b : B) X.push_back(b);
    set<int> AB;
    for (int a : A) AB.insert(a);
    for (int b : B) AB.insert(b);
    vector<int> ABv = vector<int>(AB.begin(), AB.end());

    int R = 0;
    long long PAB = 0;
    for(int x : AB)
      for(auto y : DP[q][x])
        PAB += y.second;

    R = log((double)PAB)/(epsilon*epsilon);


    map<string, ll> freqA, freqB;
    set<string> W;
    double bcw, fjw;
    long long Rp = 0ll;

    // dict.clear();
    // freqA.clear();
    // freqB.clear();
    // freqBrute.clear();
    // vmrss_brute = getCurrentRSS();
    // time_brute = current_timestamp();
    // #pragma omp parallel for schedule(static, 1)
    // for (size_t i = 0; i < ABv.size(); i++) {
    //   int tid = omp_get_thread_num();
    //   dfs(tid, ABv[i], q - 1);
    // }
    //
    // double realBC, realFJ;
    // for (auto w : freqBrute) {
    //   int u = w.first.first;
    //   string s = w.first.second;
    //   ll freq = w.second;
    //   if (A.find(u) != A.end()) {
    //     Rp += freq;
    //     freqA[s] += freq;
    //   }
    //   if (B.find(u) != B.end()) {
    //     Rp += freq;
    //     freqB[s] += freq;
    //   }
    // }
    // time_brute = current_timestamp() - time_brute;
    // tau_brute = dict.size();
    // bc_brute = realBC = bcw = BCW(dict, freqA, freqB);
    // // printf("\t\tBRUTEFORCE BCW(A,B) \t\t= %.6f\n", bcw);
    // fj_brute = realFJ = fjw = FJW(dict, freqA, freqB, Rp);
    // // printf("\t\tBRUTEFORCE FjW(A,B) \t\t= %.6f\n", fjw);
    // vmrss_brute = getCurrentRSS() - vmrss_brute;


    for(int exp = 0 ; exp < 50 ; exp++)
    {
    // printf("(%4d,%4d) R = %4d\n", ABs.first, ABs.second, R);
    // continue;

    // printf("TEST Q=[%2d] R=[%4d] (hA,hB)=(%3d,%3d):\n", q, R, ABs.first,
    // ABs.second);


    // Brute force
    // printf("\t[bruteforce]\n");


    // Base line
    // printf("\t[baseline]\n");
    freqA.clear();
    freqB.clear();
    vmrss_base = getCurrentRSS();
    time_base = current_timestamp();
    map<pair<int, string>, ll> BLsampling = baselineSampler(X, R);
    for (auto w : BLsampling) {
      int u = w.first.first;
      W.insert(w.first.second);
      if (A.find(u) != A.end()) freqA[w.first.second] += w.second;
      if (B.find(u) != B.end()) freqB[w.first.second] += w.second;
    }

    time_base = current_timestamp() - time_base;

    bc_base = bcw = BCW(W, freqA, freqB);
    // bc_base_rel = abs(bcw - realBC) / realBC;
    // printf("\t\tBASELINE BCW(A,B) \t\t= %.6f \t%.6f\n", bcw, rel);

    tau_base = W.size();
    fj_base = fjw = FJW(W, freqA, freqB, (long long)R);
    // fj_base_rel = abs(fjw - realFJ) / realFJ;
    // printf("\t\tBASELINE FJW(A,B) \t\t= %.6f \t%.6f\n", fjw, rel);
    vmrss_base = getCurrentRSS() - vmrss_base;

    // ColorfulSampler OLD
    // printf("\t[ColorfulSampler]\n");
    // time_alg3 = current_timestamp();
    // vmrss_alg3 = getCurrentRSS();
    // set<string> Sample = randomColorfulSample(X, R);
    // freqA = processFrequency(Sample, multiset<int>(A.begin(), A.end()));
    // freqB = processFrequency(Sample, multiset<int>(B.begin(), B.end()));
    //
    // time_alg3 = current_timestamp() - time_alg3;
    // tau_alg3 = Sample.size();
    // bc_alg3 = bcw = BCW(Sample, freqA, freqB);
    // bc_alg3_rel = abs(bcw - realBC) / realBC;
    // // printf("\t\tCOLORFULSAMPLER BCW(A,B) \t= %.6f \t%.6f\n", bcw, rel);
    //
    // Sample = randomColorfulSample(vector<int>(AB.begin(), AB.end()), R);
    // freqA = processFrequency(Sample, multiset<int>(A.begin(), A.end()));
    // freqB = processFrequency(Sample, multiset<int>(B.begin(), B.end()));
    // Rp = 0;
    // for (auto a : freqA) Rp += a.second;
    // for (auto b : freqB) Rp += b.second;
    // fj_alg3 = fjw = FJW(Sample, freqA, freqB, Rp);
    // fj_alg3_rel = abs(fjw - realFJ) / realFJ;
    // // printf("\t\tCOLORFULSAMPLER FJW(A,B) \t= %.6f \t%.6f\n", fjw, rel);
    // vmrss_alg3 = getCurrentRSS() - vmrss_alg3;

    // ColorfulSampler OLD
    // printf("\t[ColorfulSampler]\n");
    // set<string> BCsampling = BCSampler(A,B,R);
    // freqA = processFrequency(BCsampling, multiset<int>(A.begin(), A.end()));
    // freqB = processFrequency(BCsampling, multiset<int>(B.begin(), B.end()));
    // bcw = BCW(BCsampling, freqA, freqB);
    // rel = abs(bcw-realBC)/realBC;
    // printf("\t\tCOLORFULSAMPLER BCW(A,B) \t= %.6f \t%.6f\n", bcw, rel);
    // Rp = 0 ;
    // for(auto a : freqA) Rp += a.second;
    // for(auto b : freqB) Rp += b.second;
    // fjw = FJW(W, freqA, freqB, (long long)Rp);
    // rel = abs(fjw-realFJ)/realFJ;
    // printf("\t\tCOLORFULSAMPLER FJW(A,B) \t= %.6f \t%.6f\n", fjw, rel);

    // ColorfulSamplerPlus

    // printf("\t[ColorfulSamplerPlus]\n");
    freqA.clear();
    freqB.clear();
    W.clear();

    time_2plus = current_timestamp();
    vmrss_2plus = getCurrentRSS();

    map<pair<int, string>, ll> SamplePlus = randomColorfulSamplePlus(X, R);
    for (auto w : SamplePlus) W.insert(w.first.second);

    for (auto w : SamplePlus) {
      int u = w.first.first;
      if (A.find(u) != A.end()) {
        freqA[w.first.second] += w.second;
      }
      if (B.find(u) != B.end()) {
        freqB[w.first.second] += w.second;
      }
    }
    time_2plus = current_timestamp() - time_2plus;
    tau_2plus = W.size();
    bc_2plus = bcw = BCW(W, freqA, freqB, R);
    // bc_2plus_rel = abs(bcw - realBC) / realBC;
    // printf("\t\tCOLORFULSAMPLERPLUS BCW(A,B) \t= %.6f \t%.6f\n", bcw, rel);

    SamplePlus = randomColorfulSamplePlus(vector<int>(AB.begin(), AB.end()), R);
    W.clear();
    for (auto w : SamplePlus) W.insert(w.first.second);
    freqA.clear();
    freqB.clear();

    for (auto w : SamplePlus) {
      int u = w.first.first;
      if (A.find(u) != A.end()) {
        freqA[w.first.second] += w.second;
      }
      if (B.find(u) != B.end()) {
        freqB[w.first.second] += w.second;
      }
    }
    fj_2plus = fjw = FJW(W, freqA, freqB, (long long)R);
    // fj_2plus_rel = abs(fjw - realFJ) / realFJ;
    // printf("\t\tCOLORFULSAMPLERPLUS FJW(A,B) \t= %.6f \t%.6f\n", fjw, rel);

    vmrss_2plus = getCurrentRSS() - vmrss_2plus;

    // printf("\n");

    //                BRUTE------------------------------------|2PlUS--------------------------------|BASE-------------------------------|3---------------------------|
    // #q, R, ha, hb, BC-BRUTE, FJ-BRUTE, taureal, VmRSS, time, BC-2PLUS,
    // FJ-2PLUS, tau, VmRSS, time, BC-BASE, FJ-BASE, tau, VmRSS, time, BC-3,
    // FJ-3, tau, VmRSS, time
    printf("%.1f,", epsilon);          // Q
    printf("%2d,", q);          // Q
    printf("%4d,", R);          // R
    printf("%4zu,", A.size());  // HA
    printf("%4zu,", B.size());  // HB

    // printf("%.6f,", bc_brute);     // BC-BRUTE
    // printf("%.6f,", fj_brute);     // FJ-BRUTE
    // printf("%4d,", tau_brute);     // TAU
    // printf("%2zu,", (size_t)0);    // VmRSS
    // printf("%4llu,", time_brute);  // TIME

    printf("%.6f,", bc_2plus);               // BC-2PLUS
    // printf("%.6f,", bc_2plus_rel);           // BC-2PLUS
    printf("%.6f,", fj_2plus);               // FJ-2PLUS
    // printf("%.6f,", fj_2plus_rel);           // FJ-2PLUS
    printf("%4d,", tau_2plus);               // TAU
    // printf("%2zu,", (size_t)0);              // VmRSS
    printf("%4llu,", time_dp + time_2plus);  // TIME

    printf("%.6f,", bc_base);      // BC-BASE
    // printf("%.6f,", bc_base_rel);  // BC-BASE
    printf("%.6f,", fj_base);      // FJ-BASE
    // printf("%.6f,", fj_base_rel);  // FJ-BASE
    printf("%4d,", tau_base);      // TAU
    // printf("%2zu,", (size_t)0);    // VmRSS
    printf("%4llu", time_base);   // TIME

    // printf("%.6f,", bc_alg3);              // BC-ALG3
    // printf("%.6f,", bc_alg3_rel);          // BC-ALG3
    // printf("%.6f,", fj_alg3);              // FJ-ALG3
    // printf("%.6f,", fj_alg3_rel);          // FJ-ALG3
    // printf("%4d,", tau_alg3);              // TAU
    // printf("%2zu,", (size_t)0);            // VmRSS
    // printf("%4llu", time_dp + time_alg3);  // TIME

    printf("\n");
    }
  }

  // for(size_t i=0; i<size.size(); i++)
  // {
  //   printf("|A| = |B| = %d\n",size[i]);
  //
  //   set<int> A = set<int>(sampleV.begin(), sampleV.begin()+size[i]);
  //   set<int> B = set<int>(sampleV.end()-size[i]-1, sampleV.end());
  //   multiset<int> AB = multiset<int>(A.begin(), A.end());
  //   AB.insert(B.begin(), B.end());
  //
  //
  //   if (verbose_flag) printf("Sampling 1000 string...\n");
  //   time_a = current_timestamp();
  //   set<string> W = BCSampler(A, B, 1000);
  //   time_b = current_timestamp() - time_a;
  //   if (verbose_flag) printf("End sampling 1000 string [%llu]ms\n",time_b);
  //
  //   if (verbose_flag) printf("Calculate BCW(A,B)...\n");
  //   time_a = current_timestamp();
  //   double bcw = BCW(W,A,B);
  //   time_b = current_timestamp() - time_a;
  //   if (verbose_flag) printf("BCW(A,B) = [%.6f]  [%llu]ms\n",bcw,time_b);
  //
  //   printf("\n\n");
  // }

  // set<int> vA = set<int>(A, A + Sa);
  // set<int> vB = set<int>(B, B + Sb);
  // multiset<int> mAB = multiset<int>(A, A + Sa);
  // mAB.insert(B, B + Sb);
  //
  // set<int> AB = set<int>(mAB.begin(), mAB.end());
  // if (verbose_flag) printf("Freq{AB}(W)...\n");
  // time_a = clock();
  // for(string w : W)
  // {
  //   set<string> ws;
  //   ws.insert(w);
  //   processFrequency(ws, mAB);
  // }
  // time_b = clock()-time_a;
  // if (verbose_flag) printf("End Freq{AB}(W) [%.6f]sec\n",
  // (float)(time_b)/CLOCKS_PER_SEC);

  //
  // double sum = 0.;
  //
  // for(int i=0; i<1000; i++)
  // {
  //   if (verbose_flag) printf("Sampling strings...\n");
  //   set<string> W = BCSampler(vA, vB, 1);
  //
  //   if (verbose_flag) printf("Sampled strings:\n");
  //   for (string w : W) printf("%s\n", w.c_str());
  //
  //   if (verbose_flag) printf("Find frequency(A+B)\n");
  //   map<string, ll> freqAB = processFrequency(W, mAB);
  //   // if (verbose_flag) printf("Freq(A+B):\n");
  //   // for(auto f : freqAB)
  //   //   printf("[%10s] = [%6lld]\n", f.first.c_str(), f.second);
  //   double bcw = BCW(W,vA,vB);
  //   sum += bcw;
  //   if (verbose_flag) printf("BCW(W,A,B) = %.6f\n", bcw);
  // }
  // printf("E[BCW(W,A,B)] = %.6f\n", sum/1000.);

  return 0;
}


//
//
//
