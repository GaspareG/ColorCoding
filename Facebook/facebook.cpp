#include <bits/stdc++.h>
using namespace std;

#define MAXN 10000
#define MAXU 4039
#define TOPK 10

typedef tuple<int, int, int> label;

FILE *node, *edge, *classmate, *family, *classmate_out, *family_out;

// Induces subgraphs categories
// Classmate:  "user", "education;concentration;id","education;school;id",
// "education;degree;id", "education;school;id".
// Family "user","hometown;id", "last_name","location;id", "last_name"

map<string, vector<pair<int, int>>>
    catUserId;                          // category -> { <id node, id cat> }
map<int, pair<string, int>> userCatId;  // id node -> <category, id cat>

bool in[MAXN];
vector<int> G[MAXN];  // id node -> { id node neighbour }
int L[MAXN];          // id node -> id label

// Compute Y(x)
set<int> Y(int x, string cat) {
  vector<int> catX;
  set<int> ys;
  for (int y : G[x])  // Looking for cat node connected to x
    if (userCatId[y].first == cat) {
      /*    if( x == 63 )
          {
            printf("63 -> %d\n", y);
          }*/
      catX.push_back(y);
    }

  for (int y : catX)  // Add all users connected in catX
    for (int z : G[y])
      if (userCatId[z].first == "user") {
        ys.insert(z);
        if (x == 63) printf("63 -> %d -> %d\n", y, z);
      }

  ys.erase(x);
  return ys;
}

vector<label> labelsFrom(int x) {
  vector<label> out;
  for (int y : G[x]) {
    for (int z : G[y]) {
      if (z == x) continue;
      out.push_back(make_tuple(L[x], L[y], L[z]));
    }
  }
  return out;  // vector<label>(out.begin(), out.end());
}

double brayCurtis(int x, int y) {
  vector<label> Lx = labelsFrom(x);
  vector<label> Ly = labelsFrom(y);

  vector<label> Lxy(Lx.size() + Ly.size());
  auto it =
      set_intersection(Lx.begin(), Lx.end(), Ly.begin(), Ly.end(), Lxy.begin());
  Lxy.resize(it - Lxy.begin());

  double out = (double)((double)2. * Lxy.size()) / (Lx.size() + Ly.size());

  if (x == 63) {
    printf("BC(%d, %d) = %lg\n", x, y, out);
  }

  return out;
}

vector<int> query(int x, string cat) {
  priority_queue<pair<double, int>> Q;
  vector<int> out;
  auto ys = Y(x, cat);

  for (int y : ys) Q.push(make_pair(brayCurtis(x, y), y));

  for (int i = 0; i < 10; i++) {
    out.push_back(Q.top().second);
    Q.pop();
  }
  return out;
}

vector<int> classmateQuery(int x) { return query(x, "education;school;id"); }
vector<int> familyQuery(int x) { return query(x, "last_name"); }

int main() {
  node = fopen("graph.node", "r");
  edge = fopen("graph.edge", "r");
  classmate = fopen("classmate-query-node", "r");
  family = fopen("family-query-node", "r");

  classmate_out = fopen("classmate-query-node.out", "w");
  family_out = fopen("family-query-node.out", "w");

  if (node == NULL) return 1;
  if (edge == NULL) return 1;
  if (classmate == NULL) return 1;
  if (family == NULL) return 1;

  string classmateStrings[] = {"user", "education;concentration;id",
                               "education;school;id", "education;degree;id"};
  set<string> classmateCat(classmateStrings, classmateStrings + 4);

  string familyStrings[] = {"user", "hometown;id", "last_name", "location;id",
                            "last_name"};
  set<string> familyCat(familyStrings, familyStrings + 5);

  // Reading node from graph.node
  // lines are id TAB category TAG category id TAG 0
  while (!feof(node)) {
    int uid, cid = 0, label = 0;
    char cat[255];
    assert(1 == fscanf(node, "%d\t", &uid));

    if (uid < MAXU)  // user
    {
      strcpy(cat, "user");
      assert(0 == fscanf(node, "%*s %*d %*d\n"));
    } else  // category element
    {
      assert(2 == fscanf(node, "%s\t%*s %*s %d %*d\n", cat, &cid));
      label = uid;
    }

    in[uid] = false;
            if( classmateCat.find(string(cat)) == classmateCat.end() )
            continue;
    // if (familyCat.find(string(cat)) == familyCat.end()) continue;

    in[uid] = true;

    catUserId[string(cat)].push_back(make_pair(uid, 0));
    userCatId[uid] = make_pair(string(cat), 0);
    L[uid] = label;
    printf("<%d, %s, %d>\n", uid, cat, cid);
  }

  // Reading edge from graph.edge
  // lines are X Y
  int edges = 0;
  while (!feof(edge)) {
    int x, y;
    assert(2 == fscanf(edge, "%d %d\n", &x, &y));

    // Only edge in the induced sugraph
    if (!in[x] || !in[y]) continue;

    G[x].push_back(y);
    edges++;
  }
  printf("EDGE: %d\n", edges);

  // Reading queries from classmate
    while( !feof(classmate) )
    {
        int x;
        assert(1 == fscanf(classmate, "%d\n", &x));

        auto bcClassmate = classmateQuery(x);

        fprintf(classmate_out, "%d", x);
        for(int y : bcClassmate) fprintf(classmate_out, "\t%d", y);
        fprintf(classmate_out, "\n");
    }
    fflush(classmate_out);

  // Reading queries from Family
  /*while (!feof(family)) {
    int x;
    assert(1 == fscanf(family, "%d\n", &x));

    auto bcFamily = familyQuery(x);

    fprintf(family_out, "%d", x);
    for (int y : bcFamily) fprintf(family_out, "\t%d", y);
    fprintf(family_out, "\n");
  }
  fflush(family_out);*/

  // Closing opened files
  fclose(node);
  fclose(edge);
  fclose(classmate);
  fclose(family);
  fclose(classmate_out);
  fclose(family_out);
  return 0;
}
