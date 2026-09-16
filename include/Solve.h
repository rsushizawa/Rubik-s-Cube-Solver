#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <random>
#include <stdexcept>

#include "State.h"
#include "Transition.h"

// Cubie i sits in slot i, every orientation 0.
inline constexpr uint64_t SOLVED_STATE = 0x1C1814100C080400ULL;

// Outcome of a solver search: whether a path was found, the moves that make
// it (start --moves--> goal), and how many nodes the search expanded.
struct SearchResult {
  bool found = false;
  std::vector<std::string> moves;
  std::uint64_t expanded = 0;

  std::string notation() const {
    std::string text;
    for (std::size_t i = 0; i < moves.size(); ++i) {
      if (i) text += ' ';
      text += moves[i];
    }
    return text;
  }
};

// The moves a solver is allowed to use, plus the bookkeeping every solver
// needs regardless of search strategy: looking a move up by name, applying
// a whole sequence, finding a move's inverse, and generating scrambles.
// Kept separate from CubeSolver so scrambling never depends on having
// picked a search algorithm.
class MoveSet {
public:
  explicit MoveSet(std::vector<Move> moves) : moves_(std::move(moves)) {
    build_inverse_table();
  }

  std::size_t size() const { return moves_.size(); }
  const Move& operator[](std::size_t i) const { return moves_[i]; }

  // Index in this set of moves()[i]'s inverse, or -1 if not present.
  int inverse_of(std::size_t i) const { return inverse_[i]; }

  const Move& by_name(const std::string& name) const {
    for (const auto& m : moves_) if (name == m.name) return m;
    throw std::invalid_argument("MoveSet: unknown move '" + name + "'");
  }

  uint64_t apply_sequence(uint64_t state, const std::vector<std::string>& names) const {
    for (const auto& name : names) state = apply_move(state, by_name(name));
    return state;
  }

  // A random scramble; never turns the same face twice in a row.
  std::vector<std::string> scramble(int length,
                                    uint32_t seed = std::random_device{}()) const {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> pick(0, (int)moves_.size() - 1);
    std::vector<std::string> result;
    int last = -1;
    while ((int)result.size() < length) {
      int chosen = pick(rng);
      if (last >= 0 && moves_[chosen].name[0] == moves_[last].name[0]) continue;
      result.push_back(moves_[chosen].name);
      last = chosen;
    }
    return result;
  }

  // Quarter turns of three mutually adjacent faces (R, U, F + primes). A
  // 2x2 has no fixed centres, so these alone still reach every reachable
  // position; a smaller move set means a smaller search graph.
  static MoveSet three_adjacent_faces() {
    return MoveSet({ MOVE_R, MOVE_Rp, MOVE_U, MOVE_Up, MOVE_F, MOVE_Fp });
  }

  // All 12 quarter turns.
  static MoveSet all_faces() {
    return MoveSet(std::vector<Move>(std::begin(ALL_MOVES), std::end(ALL_MOVES)));
  }

private:
  void build_inverse_table() {
    inverse_.assign(moves_.size(), -1);
    for (std::size_t i = 0; i < moves_.size(); ++i) {
      uint64_t once = apply_move(SOLVED_STATE, moves_[i]);
      for (std::size_t j = 0; j < moves_.size(); ++j) {
        if (apply_move(once, moves_[j]) == SOLVED_STATE) { inverse_[i] = (int)j; break; }
      }
    }
  }

  std::vector<Move> moves_;
  std::vector<int> inverse_;
};

// Interface every search strategy implements. A concrete solver owns
// whatever data structure its search needs (a visited-state hash map, a
// pattern database, ...); CubeSolver itself holds none, only the move set
// every strategy searches over.
class CubeSolver {
public:
  explicit CubeSolver(MoveSet moveSet) : moves_(std::move(moveSet)) {}
  virtual ~CubeSolver() = default;

  virtual SearchResult solve(uint64_t start, uint64_t goal = SOLVED_STATE) const = 0;

  const MoveSet& moves() const { return moves_; }

protected:
  MoveSet moves_;
};
