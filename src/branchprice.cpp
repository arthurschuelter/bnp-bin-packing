// Set partitioning branch-and-price. Only continuous variables reach Gurobi.
#include "branchprice.h"
#include "gurobi_c++.h"
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <numeric>
#include <queue>
#include <random>
#include <set>
#include <stdexcept>

namespace
{
using Clock = std::chrono::steady_clock;
using Pattern = std::vector<int>;
constexpr double RC_EPS = 1e-7;
constexpr double INT_EPS = 1e-6;
struct TimeExpired
{
};
void checkTime(Clock::time_point deadline)
{
	if (Clock::now() >= deadline)
		throw TimeExpired{};
}

struct Node
{
	std::vector<std::pair<int, int>> together, apart;
	int bound;
	unsigned long long id;
};
struct BestBound
{
	bool operator()(const Node &a, const Node &b) const
	{
		return a.bound != b.bound ? a.bound > b.bound : a.id > b.id;
	}
};
struct Groups
{
	std::vector<Pattern> items;
	std::vector<long long> size;
	std::vector<std::vector<bool>> conflict;
	std::vector<int> group_of;
	bool valid = true;
	Groups(const BinPackingData &d, const Node &node, Clock::time_point deadline)
	{
		std::vector<int> parent(d.n + 1);
		std::iota(parent.begin(), parent.end(), 0);
		std::function<int(int)> root = [&](int i)
		{ return parent[i] == i ? i : parent[i] = root(parent[i]); };
		for (auto [i, j] : node.together)
			parent[root(i)] = root(j);
		std::map<int, int> ids;
		group_of.resize(d.n + 1);
		for (int i = 1; i <= d.n; ++i)
		{
			checkTime(deadline);
			int r = root(i);
			auto it = ids.emplace(r, static_cast<int>(ids.size())).first;
			int g = it->second;
			if (g == static_cast<int>(items.size()))
			{
				items.emplace_back();
				size.push_back(0);
			}
			group_of[i] = g;
			items[g].push_back(i);
			if (d.size[i] > d.C - size[g])
			{
				valid = false;
				return;
			}
			size[g] += d.size[i];
		}
		conflict.assign(items.size(), std::vector<bool>(items.size(), false));
		auto forbid = [&](int i, int j)
		{
			checkTime(deadline);
			int a = group_of[i], b = group_of[j];
			if (a == b)
				valid = false;
			else
				conflict[a][b] = conflict[b][a] = true;
		};
		for (const auto &s : d.cls)
			for (size_t i = 0; i < s.size(); ++i)
				for (size_t j = i + 1; j < s.size(); ++j)
					forbid(s[i], s[j]);
		for (auto [i, j] : node.apart)
			forbid(i, j);
	}
	Pattern expand(const Pattern &p) const
	{
		Pattern result;
		for (int g : p)
			result.insert(result.end(), items[g].begin(), items[g].end());
		std::sort(result.begin(), result.end());
		return result;
	}
	bool admissible(const Pattern &p, Pattern &out) const
	{
		std::vector<int> counts(items.size(), 0);
		for (int i : p)
			++counts[group_of[i]];
		out.clear();
		for (size_t g = 0; g < counts.size(); ++g)
		{
			if (!counts[g])
				continue;
			if (counts[g] != static_cast<int>(items[g].size()))
				return false;
			for (int h : out)
				if (conflict[g][h])
					return false;
			out.push_back(static_cast<int>(g));
		}
		return !out.empty();
	}
};

class DeadlineCallback : public GRBCallback
{
	Clock::time_point deadline;
	void callback() override
	{
		if (Clock::now() >= deadline)
			abort();
	}

  public:
	explicit DeadlineCallback(Clock::time_point d) : deadline(d) {}
};

// Fractional knapsack, ignoring conflicts, upper-bounds all feasible pricing
// patterns. Positive duals only: removing a nonpositive group preserves validity.
struct Pricing
{
	const Groups &g;
	const std::vector<double> &u;
	long long capacity;
	Clock::time_point deadline;
	Pattern order, chosen, best_pattern;
	long double best = 0, upper = 0;
	bool complete = true;
	Pricing(const Groups &groups, const std::vector<double> &duals, long long c,
	        Clock::time_point d)
	    : g(groups), u(duals), capacity(c), deadline(d)
	{
		for (size_t i = 0; i < u.size(); ++i)
			if (u[i] > 0)
				order.push_back(static_cast<int>(i));
		std::sort(order.begin(), order.end(),
		          [&](int a, int b)
		          {
			          long double x = static_cast<long double>(u[a]) * g.size[b];
			          long double y = static_cast<long double>(u[b]) * g.size[a];
			          return x != y ? x > y : a < b;
		          });
		upper = fractional(0, capacity);
	}
	long double fractional(size_t pos, long long remaining) const
	{
		long double value = 0;
		for (size_t p = pos; p < order.size() && remaining > 0; ++p)
		{
			int i = order[p];
			if (g.size[i] <= remaining)
			{
				remaining -= g.size[i];
				value += u[i];
			}
			else
			{
				value += static_cast<long double>(u[i]) * remaining / g.size[i];
				break;
			}
		}
		return value;
	}
	void dfs(size_t pos, long long remaining, long double value)
	{
		if (Clock::now() >= deadline)
		{
			complete = false;
			return;
		}
		if (value > best)
		{
			best = value;
			best_pattern = chosen;
		}
		if (pos == order.size() || value + fractional(pos, remaining) <= best + 1e-10L)
			return;
		int i = order[pos];
		bool ok = g.size[i] <= remaining;
		for (int j : chosen)
			if (g.conflict[i][j])
			{
				ok = false;
				break;
			}
		if (ok)
		{
			chosen.push_back(i);
			dfs(pos + 1, remaining - g.size[i], value + u[i]);
			chosen.pop_back();
			if (!complete)
				return;
		}
		dfs(pos + 1, remaining, value);
	}
	void exact()
	{
		dfs(0, capacity, 0);
		// Account for the small pruning tolerance and floating arithmetic.
		if (complete)
			upper = best + 1e-8L;
	}
};

int certifiedBound(const std::vector<double> &u, long double alpha, int inherited)
{
	long double sum = 0;
	for (double x : u)
		sum += x;
	long double safe_alpha = std::max(1.0L, alpha) + 1e-8L;
	return std::max(inherited, static_cast<int>(std::ceil(sum / safe_alpha - INT_EPS)));
}

struct Stats
{
	size_t nodes = 0, lp = 0, heuristic = 0, exact = 0, branches = 0;
};
} // namespace

void BranchAndPrice(BinPackingData &data, Clock::time_point deadline)
{
	std::priority_queue<Node, std::vector<Node>, BestBound> open;
	unsigned long long next_id = 1;
	open.push(Node{{}, {}, data.lower_bound, 0});
	std::set<Pattern> pool;
	for (int i = 1; i <= data.n; ++i)
		pool.insert({i});
	for (auto bin : data.best_solution)
	{
		std::sort(bin.begin(), bin.end());
		pool.insert(bin);
	}
	Stats stats;
	std::mt19937_64 rng(data.seed);
	try
	{
		GRBEnv env(true);
		env.set(GRB_IntParam_OutputFlag, 1);
		env.set(GRB_IntParam_Threads, 1);
		env.start();
		DeadlineCallback callback(deadline);
		while (!open.empty() && Clock::now() < deadline)
		{
			if (open.top().bound >= data.best_value)
			{
				open.pop();
				continue;
			}
			Node node = open.top();
			open.pop();
			// Keep the current node in the frontier until it has been resolved or
			// replaced by BOTH children. Thus timeouts cannot lose a subtree bound.
			open.push(node);
			++stats.nodes;
			Groups groups(data, node, deadline);
			if (!groups.valid)
			{
				open.pop();
				continue;
			}
			GRBModel model(env);
			model.setCallback(&callback);
			model.set(GRB_IntParam_Method, 1); // dual simplex; incremental columns
			model.set(GRB_DoubleParam_FeasibilityTol, 1e-9);
			model.set(GRB_DoubleParam_OptimalityTol, 1e-9);
			std::vector<GRBConstr> rows;
			for (size_t g = 0; g < groups.items.size(); ++g)
				rows.push_back(model.addConstr(GRBLinExpr(0.0) == 1.0));
			model.update();
			std::set<Pattern> local;
			std::vector<Pattern> patterns;
			std::vector<GRBVar> vars;
			auto add = [&](Pattern p)
			{
				std::sort(p.begin(), p.end());
				if (!local.insert(p).second)
					return false;
				GRBColumn column;
				for (int g : p)
					column.addTerm(1.0, rows[g]);
				vars.push_back(model.addVar(0.0, GRB_INFINITY, 1.0, GRB_CONTINUOUS, column));
				patterns.push_back(p);
				pool.insert(groups.expand(p));
				return true;
			};
			for (size_t g = 0; g < groups.items.size(); ++g)
			{
				checkTime(deadline);
				add({static_cast<int>(g)});
			}
			// Snapshot: adding a column also inserts its expansion into the pool.
			std::vector<Pattern> snapshot(pool.begin(), pool.end());
			for (const auto &p : snapshot)
			{
				checkTime(deadline);
				Pattern q;
				if (groups.admissible(p, q))
					add(q);
			}
			bool solved = false, stopped = false, branched = false;
			while (Clock::now() < deadline)
			{
				model.set(
				    GRB_DoubleParam_TimeLimit,
				    std::max(0.0, std::chrono::duration<double>(deadline - Clock::now()).count()));
				model.optimize();
				++stats.lp;
				if (model.get(GRB_IntAttr_Status) != GRB_OPTIMAL)
				{
					stopped = true;
					break;
				}
				std::vector<double> dual(rows.size());
				for (size_t g = 0; g < rows.size(); ++g)
					dual[g] = rows[g].get(GRB_DoubleAttr_Pi);
				Pricing pricing(groups, dual, data.C, deadline);
				node.bound = certifiedBound(dual, pricing.upper, node.bound);
				if (node.bound >= data.best_value)
				{
					solved = true;
					break;
				}
				bool added = false;
				// Always run the heuristic first; vary value/density/randomized order.
				for (int round = 0; round < 12 && Clock::now() < deadline; ++round)
				{
					Pattern order = pricing.order;
					if (round == 1)
						std::sort(order.begin(), order.end(), [&](int a, int b)
						          { return dual[a] != dual[b] ? dual[a] > dual[b] : a < b; });
					else if (round > 1)
						std::shuffle(order.begin(), order.end(), rng);
					Pattern p;
					long long remaining = data.C;
					long double value = 0;
					for (int g : order)
					{
						if (groups.size[g] > remaining)
							continue;
						bool ok = true;
						for (int h : p)
							if (groups.conflict[g][h])
							{
								ok = false;
								break;
							}
						if (ok)
						{
							p.push_back(g);
							remaining -= groups.size[g];
							value += dual[g];
						}
					}
					if (value > 1 + RC_EPS && add(p))
					{
						added = true;
						++stats.heuristic;
					}
				}
				if (added)
					continue;
				if (Clock::now() >= deadline)
				{
					stopped = true;
					break;
				}
				++stats.exact;
				pricing.exact();
				node.bound = certifiedBound(dual, pricing.upper, node.bound);
				if (node.bound >= data.best_value)
				{
					solved = true;
					break;
				}
				if (pricing.best > 1 + RC_EPS)
				{
					if (!add(pricing.best_pattern))
						throw std::runtime_error("pricing repeated a negative reduced-cost column");
					if (!pricing.complete)
					{
						stopped = true;
						break;
					}
					continue;
				}
				if (!pricing.complete)
				{
					stopped = true;
					break;
				}
				// Exact pricing certified convergence. Read the primal only now.
				std::vector<double> x(vars.size());
				bool integral = true;
				for (size_t p = 0; p < vars.size(); ++p)
				{
					x[p] = vars[p].get(GRB_DoubleAttr_X);
					if (std::abs(x[p] - std::round(x[p])) > INT_EPS)
						integral = false;
				}
				if (integral)
				{
					std::vector<Pattern> bins;
					for (size_t p = 0; p < x.size(); ++p)
						if (x[p] > 0.5)
							bins.push_back(groups.expand(patterns[p]));
					std::string error;
					if (!data.CheckSolution(bins, error))
						throw std::runtime_error(error);
					data.UpdateBest(bins);
					solved = true;
					break;
				}
				std::map<std::pair<int, int>, double> phi;
				for (size_t p = 0; p < x.size(); ++p)
					if (x[p] > 1e-9)
						for (size_t a = 0; a < patterns[p].size(); ++a)
							for (size_t b = a + 1; b < patterns[p].size(); ++b)
								phi[{patterns[p][a], patterns[p][b]}] += x[p];
				std::pair<int, int> pair{-1, -1};
				double distance = 1;
				for (auto entry : phi)
				{
					double v = entry.second;
					if (v > INT_EPS && v < 1 - INT_EPS && std::abs(v - 0.5) < distance)
					{
						pair = entry.first;
						distance = std::abs(v - 0.5);
					}
				}
				if (pair.first < 0)
					throw std::runtime_error("no fractional Ryan-Foster pair (numerical issue)");
				int i = groups.items[pair.first].front(), j = groups.items[pair.second].front();
				Node together = node, apart = node;
				together.together.emplace_back(i, j);
				together.id = next_id++;
				apart.apart.emplace_back(i, j);
				apart.id = next_id++;
				open.pop();
				open.push(std::move(together));
				open.push(std::move(apart));
				++stats.branches;
				solved = true;
				branched = true;
				break;
			}
			if (branched)
				continue; // parent already replaced by both children
			open.pop();
			if (!solved)
			{
				open.push(node);
				stopped = true;
			}
			if (stopped)
				break;
		}
	}
	catch (const TimeExpired &)
	{
		// The current node is still in the frontier with its inherited bound.
	}
	catch (const GRBException &e)
	{
		std::cerr << "Gurobi: " << e.getMessage() << " (" << e.getErrorCode()
		          << "). Returning incumbent and certified frontier bound.\n";
	}
	catch (const std::exception &e)
	{
		std::cerr << "Search interrupted: " << e.what() << ". Returning certified frontier.\n";
	}
	data.lower_bound =
	    open.empty() ? data.best_value
		             : std::max(data.lower_bound, std::min(data.best_value, open.top().bound));
	std::cout << "[B&P] nodes=" << stats.nodes << " LPs=" << stats.lp
	          << " heuristic_columns=" << stats.heuristic << " exact_calls=" << stats.exact
	          << " branches=" << stats.branches << " pool=" << pool.size()
	          << " LB=" << data.lower_bound << " UB=" << data.best_value << '\n';
}
