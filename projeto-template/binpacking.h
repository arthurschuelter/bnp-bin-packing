//----------------------------------------------------------------------
// Bin Packing Problem with Conflict Classes.
//
// This header defines BinPackingData, the type that encapsulates the whole
// input of an instance, together with the value and the best solution found
// so far (in the same spirit as TSP_Data in ex_tsp.cpp). A pointer/reference
// to this object is all that the solver routine needs.
//
// Conventions: the items are I = {1,...,n} (vectors indexed by items have
// n+1 positions; position 0 is not used), and the conflict classes are
// S_0,...,S_{k-1} (index t in 0..k-1). Two items i != j cannot be packed in
// the same bin if there is a class t with i,j in S_t.
//----------------------------------------------------------------------
#ifndef BINPACKING_H
#define BINPACKING_H

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class BinPackingData {
public:
  // ---------------------------- input data -----------------------------
  int n;                                 // number of items
  long long C;                           // capacity of each bin
  int k;                                 // number of conflict classes
  std::vector<long long> size;           // size[i], 0 < size[i] <= C, i=1..n
  std::vector<std::vector<int> > cls;    // cls[t] = items of the class S_t
  std::vector<std::vector<int> > classes_of; // classes_of[i] = classes containing i

  // ----------------- parameters of the run (set by main) ----------------
  unsigned seed;                         // seed of the random number generator

  // ------------------- best solution found so far -----------------------
  // best_value is the number of bins of best_solution; it is INT_MAX-like
  // (n+1) while no solution is known. best_solution[b] is the list of items
  // packed in the bin b. lower_bound is a valid lower bound on the optimum.
  int best_value;
  std::vector<std::vector<int> > best_solution;
  int lower_bound;

  BinPackingData() : n(0), C(0), k(0), seed(1), best_value(1), lower_bound(0) {}

  // Must be called after n, C, k, size and cls are filled.
  void Prepare() {
    classes_of.assign(n + 1, std::vector<int>());
    for (int t = 0; t < k; t++)
      for (size_t a = 0; a < cls[t].size(); a++)
        classes_of[cls[t][a]].push_back(t);
    best_value = n + 1;     // "no solution yet" (a solution has at most n bins)
    best_solution.clear();
    lower_bound = TrivialLowerBound();
  }

  bool HasSolution() const { return !best_solution.empty() || n == 0; }

  // True if the items i and j (i != j) cannot be in the same bin.
  bool Conflict(int i, int j) const {
    if (i == j) return false;
    const std::vector<int> &a = classes_of[i], &b = classes_of[j];
    size_t p = 0, q = 0;                 // both lists are sorted (see Prepare)
    while (p < a.size() && q < b.size()) {
      if (a[p] == b[q]) return true;
      if (a[p] < b[q]) p++; else q++;
    }
    return false;
  }

  // A simple valid lower bound: the total size divided by C (rounded up), and
  // the size of the largest class (its items must be in different bins).
  int TrivialLowerBound() const {
    long long total = 0;
    for (int i = 1; i <= n; i++) total += size[i];
    int lb = (int)((total + C - 1) / C);
    for (int t = 0; t < k; t++) lb = std::max(lb, (int)cls[t].size());
    return lb;
  }

  // Updates the best solution if the given packing is better. Returns true
  // if it was stored.
  bool UpdateBest(const std::vector<std::vector<int> > &bins) {
    if ((int)bins.size() >= best_value) return false;
    best_solution = bins;
    best_value = (int)bins.size();
    return true;
  }

  // Checks whether `bins` is a feasible packing: every item appears in exactly
  // one bin, no bin exceeds C, and no bin contains two items of a same class.
  // On failure, a message describing the first violation is stored in `msg`.
  bool CheckSolution(const std::vector<std::vector<int> > &bins, std::string &msg) const {
    std::vector<int> seen(n + 1, 0);
    for (size_t b = 0; b < bins.size(); b++) {
      long long load = 0;
      for (size_t a = 0; a < bins[b].size(); a++) {
        int i = bins[b][a];
        if (i < 1 || i > n) {
          std::ostringstream o; o << "bin " << b + 1 << " contains invalid item " << i;
          msg = o.str(); return false; }
        if (++seen[i] > 1) {
          std::ostringstream o; o << "item " << i << " appears more than once";
          msg = o.str(); return false; }
        load += size[i];
      }
      if (load > C) {
        std::ostringstream o; o << "bin " << b + 1 << " has load " << load << " > C = " << C;
        msg = o.str(); return false; }
      for (size_t a = 0; a < bins[b].size(); a++)
        for (size_t c = a + 1; c < bins[b].size(); c++)
          if (Conflict(bins[b][a], bins[b][c])) {
            std::ostringstream o;
            o << "bin " << b + 1 << " contains the conflicting items "
              << bins[b][a] << " and " << bins[b][c];
            msg = o.str(); return false; }
    }
    for (int i = 1; i <= n; i++)
      if (seen[i] != 1) {
        std::ostringstream o; o << "item " << i << " is not packed";
        msg = o.str(); return false; }
    return true;
  }
};

#endif
