#pragma once

#include <cstdio>
#include <deque>
#include <unordered_map>
#include <algorithm>

#include "Solve.h"

/**
 * breadth-first search.
 */
class BfsSolver : public CubeSolver {
public:
  using CubeSolver::CubeSolver;

  /**
   * Search for a shortest path from `start` to `goal`.
   *
   * Parameters
   * ----------
   * start: uint64_t
   *     Starting state.
   * goal: uint64_t
   *     Target state (default is SOLVED_STATE).
   *
   * Returns
   * -------
   * SearchResult
   *     Shortest move sequence from `start` to `goal`, if one exists.
   */
  SearchResult solve(uint64_t start, uint64_t goal = SOLVED_STATE) const override {
    SearchResult result;
    if (is_goal(start, goal)) { result.found = true; return result; }

    std::deque<uint64_t> frontier{start};
    ParentLinks parentOf;
    parentOf.reserve(1u << 20);
    parentOf.emplace(start, Link{start, -1});

    while (!frontier.empty()) {
      uint64_t current = frontier.front();
      frontier.pop_front();
      ++result.expanded;
      std::printf("BFS queue: size=%zu expanded=%llu\n", frontier.size(),
                  (unsigned long long)result.expanded);
      for (std::size_t i = 0; i < moves_.size(); ++i) {
        uint64_t next = apply_move(current, moves_[i]);
        if (!parentOf.emplace(next, Link{current, (int)i}).second) continue;
        if (is_goal(next, goal)) {
          result.found = true;
          result.moves = trace_path(parentOf, start, next);
          return result;
        }
        frontier.push_back(next);
      }
    }
    return result;
  }

private:
  struct Link { uint64_t parent; int moveIndex; };
  using ParentLinks = std::unordered_map<uint64_t, Link>;

  /**
   * Walk parent links back from `goal` to `start` and reverse them into a
   * forward move list.
   *
   * Parameters
   * ----------
   * parentOf: const ParentLinks&
   *     child -> (parent, move index) links, as built by solve().
   * start: uint64_t
   *     Search root.
   * goal: uint64_t
   *     State to trace back from.
   *
   * Returns
   * -------
   * std::vector<std::string>
   *     Move names, start --moves--> goal.
   */
  std::vector<std::string> trace_path(const ParentLinks& parentOf,
                                      uint64_t start, uint64_t goal) const {
    std::vector<std::string> path;
    for (uint64_t state = goal; state != start; ) {
      const Link& link = parentOf.at(state);
      path.push_back(moves_[link.moveIndex].name);
      state = link.parent;
    }
    std::reverse(path.begin(), path.end());
    return path;
  }
};
