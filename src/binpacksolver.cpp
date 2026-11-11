//----------------------------------------------------------------------
// Solver routine for the Bin Packing Problem with Conflict Classes.
//
// Preserves the supplied randomized First Fit heuristic for the initial
// incumbent, then calls the exact branch-and-price implemented separately.
//----------------------------------------------------------------------
#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>
#include "binpacksolver.h"
#include "branchprice.h"

using namespace std;

static mt19937_64 rng(1);   // seeded in SolveBinPacking from data.seed

static double RandomUniform01() {
  return uniform_real_distribution<double>(0.0, 1.0)(rng);
}

// Priority of an item in a round of the heuristic; items are packed in
// decreasing order of priority.
//   size    : size of the item divided by C, in (0,1];
//   nclasses: number of classes that contain the item divided by the maximum,
//             over all items, of that number, in [0,1];
//   bias    : "tendency" number in [0,1].
// For bias = 0 the priority is the size, so the order is First Fit Decreasing.
// For bias = 1 the priority is nclasses, so the items that take part in more
// conflict classes come first. For bias = 1/2 the priority is a random number
// that depends neither on the size nor on the number of classes. In between,
// the priority is a convex combination of the random number and the size (for
// bias < 1/2) or the random number and nclasses (for bias > 1/2).
static double ItemPriority(double size, double nclasses, double bias) {
  double r = RandomUniform01();
  if (bias <= 0.5) {
    double lambda = 1.0 - 2.0 * bias;           // 1 at bias=0, 0 at bias=1/2
    return lambda * size + (1.0 - lambda) * r;
  } else {
    double mu = 2.0 * bias - 1.0;               // 0 at bias=1/2, 1 at bias=1
    return mu * nclasses + (1.0 - mu) * r;
  }
}

// One round: sorts the items by ItemPriority (ties: larger size first) and
// packs them in First Fit style, never putting two conflicting items in the same
// bin. A new bin is opened only if no open bin accepts the item. Always
// produces a feasible packing, since each item has size <= C.
static bool FirstFitRound(const BinPackingData &data, double bias,
                          vector<vector<int> > &bins,
                          chrono::steady_clock::time_point deadline) {
  int n = data.n;
  int maxcls = 0;
  for (int i = 1; i <= n; i++) maxcls = max(maxcls, (int)data.classes_of[i].size());

  vector<double> prio(n + 1, 0.0);
  vector<int> order;
  for (int i = 1; i <= n; i++) {
    double s = (double)data.size[i] / (double)data.C;
    double c = maxcls > 0 ? (double)data.classes_of[i].size() / maxcls : 0.0;
    prio[i] = ItemPriority(s, c, bias);
    order.push_back(i);
  }
  sort(order.begin(), order.end(), [&](int a, int b) {
    if (prio[a] != prio[b]) return prio[a] > prio[b];
    if (data.size[a] != data.size[b]) return data.size[a] > data.size[b];
    return a < b;
  });

  bins.clear();
  vector<long long> load;
  vector<vector<char> > used;       // used[b][t] = 1 if bin b has an item of class t
  for (size_t p = 0; p < order.size(); p++) {
    if (chrono::steady_clock::now() >= deadline) return false;
    int i = order[p];
    int chosen = -1;
    for (size_t b = 0; b < bins.size() && chosen < 0; b++) {
      if (data.size[i] > data.C - load[b]) continue;
      bool ok = true;
      for (size_t a = 0; a < data.classes_of[i].size() && ok; a++)
        if (used[b][data.classes_of[i][a]]) ok = false;
      if (ok) chosen = (int)b;
    }
    if (chosen < 0) {
      chosen = (int)bins.size();
      bins.push_back(vector<int>());
      load.push_back(0);
      used.push_back(vector<char>(data.k, 0));
    }
    bins[chosen].push_back(i);
    load[chosen] += data.size[i];
    for (size_t a = 0; a < data.classes_of[i].size(); a++)
      used[chosen][data.classes_of[i][a]] = 1;
  }
  return true;
}

// Randomized First Fit Decreasing with conflicts. In each of at most
// `max_rounds` rounds, a bias in [0,1] is drawn uniformly at random and used to
// order all the items of that round. The best packing found is kept in data.
static void RandomizedFFD(BinPackingData &data, int max_rounds, double time_limit,
                          const chrono::steady_clock::time_point &start) {
  vector<vector<int> > bins;
  for (int round = 0; round < max_rounds; round++) {
    double elapsed = chrono::duration<double>(chrono::steady_clock::now() - start).count();
    if (elapsed >= time_limit) break;    // respect the initialization budget
    if (data.best_value <= data.lower_bound) break;   // optimal
    double bias = RandomUniform01();
    if (!FirstFitRound(data, bias, bins, start +
        chrono::duration_cast<chrono::steady_clock::duration>(
            chrono::duration<double>(time_limit)))) break;
    if (data.UpdateBest(bins))
      cout << "[Heuristic: randomized FFD] round " << round + 1 << " (bias "
           << bias << "): " << data.best_value << " bins" << endl;
  }
}

void SolveBinPacking(BinPackingData &data, double time_limit) {
  const auto start = chrono::steady_clock::now();
  const auto deadline = start + chrono::duration_cast<chrono::steady_clock::duration>(
      chrono::duration<double>(time_limit));
  rng.seed(data.seed);
  // A fallback exists even when T=0. The supplied randomized FFD remains the
  // source of the initial incumbent and its bins seed the restricted master.
  vector<vector<int>> fallback;
  for (int i = 1; i <= data.n; ++i) fallback.push_back({i});
  data.UpdateBest(fallback);
  data.lower_bound = max(data.lower_bound, data.TrivialLowerBound());
  RandomizedFFD(data, 1000, min(0.25, time_limit * 0.05), start);
  if (data.best_value > data.lower_bound && chrono::steady_clock::now() < deadline)
    BranchAndPrice(data, deadline);
}
