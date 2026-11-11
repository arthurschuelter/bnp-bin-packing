#ifndef BRANCHPRICE_H
#define BRANCHPRICE_H
#include "binpacking.h"
#include <chrono>
void BranchAndPrice(BinPackingData &data,
                    std::chrono::steady_clock::time_point deadline);
#endif
