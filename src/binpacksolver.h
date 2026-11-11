#ifndef BINPACKSOLVER_H
#define BINPACKSOLVER_H

#include "binpacking.h"

// Solves (or tries to solve) the instance in `data` within `time_limit` seconds
// (wall clock). On return, data.best_value / data.best_solution hold the best
// packing found and data.lower_bound the best lower bound known. The routine
// must only store feasible packings (see BinPackingData::CheckSolution).
void SolveBinPacking(BinPackingData &data, double time_limit);

#endif
