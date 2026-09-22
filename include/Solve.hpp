#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "State.hpp"
#include "Transition.hpp"

/**
 * Generate a random scramble; never turns the same face twice in a row.
 *
 * Parameters
 * ----------
 * moves: const std::vector<Move>&
 *     Moves the scramble may draw from, e.g. ALL_MOVES.
 * length: int
 *     Number of moves to generate.
 * seed: uint32_t
 *     RNG seed (default is nondeterministic).
 *
 * Returns
 * -------
 * std::vector<std::string>
 *     `length` move names.
 */
inline std::vector<std::string>
scramble(const std::vector<Move> &moves, int length,
         uint32_t seed = std::random_device{}()) {
  std::mt19937 rng(seed);
  std::uniform_int_distribution<int> pick(0, (int)moves.size() - 1);
  std::vector<std::string> result;
  int last = -1;
  while ((int)result.size() < length) {
    int chosen = pick(rng);
    if (last >= 0 && moves[chosen].name[0] == moves[last].name[0])
      continue;
    result.push_back(moves[chosen].name);
    last = chosen;
  }
  return result;
}

/**
 * Outcome of a CubeSolver search.
 *
 * Attributes
 * ----------
 * found: bool
 *     Whether a path from start to goal was found.
 * moves: std::vector<std::string>
 *     Move names, applied left-to-right: start --moves--> goal.
 * expanded: std::uint64_t
 *     Nodes expanded during the search (statistic, not used for
 *     correctness).
 */
struct SearchResult {
  bool found = false;
  std::vector<std::string> moves;
  std::uint64_t expanded = 0;

  /**
   * Render the move list as a single space-separated string.
   *
   * Returns
   * -------
   * std::string
   *     The moves in order, e.g. "R U R' U'".
   */
  std::string notation() const {
    std::string text;
    for (std::size_t i = 0; i < moves.size(); ++i) {
      if (i)
        text += ' ';
      text += moves[i];
    }
    return text;
  }
};

/**
 * Replay a solver's result from `start` and confirm it actually reaches
 * `goal` -- solve() itself never checks this; a caller that wants the
 * guarantee calls this explicitly.
 *
 * Parameters
 * ----------
 * moves: const std::vector<Move>&
 *     The move set result.moves' names are drawn from.
 * start: uint64_t
 *     State the solver was asked to solve from.
 * result: const SearchResult&
 *     Its return value.
 * goal: uint64_t
 *     Target passed to solve() (default SOLVED_STATE).
 *
 * Returns
 * -------
 * bool
 *     True if replaying result.moves from start reaches goal (or, when
 *     goal is SOLVED_STATE, any of its 24 rotations -- see is_goal()).
 */
inline bool verify_solution(const std::vector<Move> &moves, uint64_t start,
                            const SearchResult &result,
                            uint64_t goal = SOLVED_STATE) {
  if (!result.found)
    return false;
  uint64_t state = start;
  for (const std::string &name : result.moves) {
    bool applied = false;
    for (const Move &m : moves) {
      if (name == m.name) {
        state = apply_move(state, m);
        applied = true;
        break;
      }
    }
    if (!applied)
      return false;
  }
  return is_goal(state, goal);
}

/**
 * Interface every search strategy implements.
 */
class CubeSolver {
public:
  /**
   * Parameters
   * ----------
   * moves: std::vector<Move>
   *     Moves this solver's search is allowed to use.
   */
  explicit CubeSolver(std::vector<Move> moves) : moves_(std::move(moves)) {}
  virtual ~CubeSolver() = default;

  /**
   * Search for a path from `start` to `goal`.
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
   *     The outcome of the search; see SearchResult.
   */
  virtual SearchResult solve(uint64_t start,
                             uint64_t goal = SOLVED_STATE) const = 0;

  const std::vector<Move> &moves() const { return moves_; }

protected:
  std::vector<Move> moves_;
};
