#include <bits/stdc++.h>

using namespace std;
typedef long long ll;

unsigned int N, M, k, kp;
int *color;
vector<int> *G;

inline int nextInt()
{
    int r;
    scanf("%d",&r);
    return r;
}

inline bool getBit(ll n, int pos){ return ((n >> pos) & 1) == 1; }
inline ll   setBit(ll n, int pos){ return n |= 1 << pos; }
inline ll clearBit(ll n, int pos){ return n &= ~(1 << pos); }

inline void randomColor()
{
    for(unsigned int i=0; i<=N; i++) color[i] = rand() % kp;
}

struct SimpleHash {
    size_t operator()(const std::pair<int, int>& p) const {
        return p.first ^ p.second;
    }
};

unordered_map< pair<int, ll>, vector<int>, SimpleHash > link;
inline void addLink(int x, ll C, int j)
{
    // printf("[%d, %llu] -> %d\n", x, C, j);
    auto key = make_pair(x, C);
    if( link.find(key) == link.end() )
      link[key] = vector<int>();
    link[key].push_back(j);
}

vector<int> empty;
vector<int> H(int x, ll C)
{
  //if( link.find(make_pair(x, C)) == link.end() )
  //  return empty;
  return link[ make_pair(x, C) ];
}

ll cont = 0 ;
void list_k_path(vector<int> ps, ll cs, int x)
{
  vector<int> N = H(x, cs);
  for(int v : N)
  {
    if( (ps.size() + 2) == k )
    {
      ps.push_back(v);
      for(int j : ps) printf("%d ", j);
      printf("\n");
      ps.pop_back();
    }
    else
    {
      ps.push_back(v);
      list_k_path(ps, setBit(cs, color[v]), v);
      ps.pop_back();
    }
  }
}

void list_k_path_c(vector<int> ps, ll cs, int x, int kp)
{
  vector<int> N = H(x, cs);
  if( kp + 2 == k ) cont += N.size();
  else 
    for(int v : N)
      list_k_path_c(ps, setBit(cs, color[v]), v, kp+1);
}

#define MAXK 16
#define MAXN 10000
unordered_set<ll> DP[MAXK][MAXN];

void processDP()
{
  DP[1][N].insert( setBit(0ll, color[N]) );
  for(unsigned int i = 2 ; i <= k ; i++ )
    for(unsigned int j = 0 ; j <= N ; j++ )
      for(int x : G[j])
        for(ll C : DP[i-1][x])
          if( !getBit(C, color[j]) )
              DP[i][j].insert( setBit(C, color[j]) );
}

void backProp()
{
  vector<ll> toDel;
  for(int i = k-1; i >= 0; i--)
  {
    for(unsigned int x = 0 ; x <= N; x++)
    {
      toDel.clear();
      for(ll C : DP[i][x])
      {
        bool find = false;
        for(int j : G[x])
        {
          if( getBit(C, color[j]) ) continue;

          if( DP[i+1][j].find( setBit(C, color[j]) ) != DP[i+1][j].end() )
          {
            find = true;
            addLink(x, C, j);
          }
        }
        if( !find ) toDel.push_back(C);
      }
      for(ll C : toDel) DP[i][x].erase(C);
    }
  }
}

int main(int argc, char **argv)
{
  if( argc < 3 )
  {
    printf("Usage: %s k kp", argv[0]);
    return 1;
  }

  srand(42);

  N = nextInt();
  M = nextInt();

  k  = atol(argv[1]);
  kp = atol(argv[2]);

  color = new int[N+1];
  G = new vector<int>[N+1];

  for(unsigned int i=0; i<M; i++)
  {
    int a = nextInt();
    int b = nextInt();
    G[a].push_back(b);
    G[b].push_back(a);
  }

  for(unsigned int i=0; i<N; i++)
  {
    G[N].push_back(i);
    G[i].push_back(N);
  }

  randomColor();
  color[N] = kp;
  k++;

  processDP();
  backProp();
  list_k_path_c(vector<int>(), setBit(0ll, color[N]), N, 0);

  printf("%llu\n", cont);

  return 0;
}
