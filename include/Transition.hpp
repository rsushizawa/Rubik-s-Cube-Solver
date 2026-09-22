#pragma once

#include "State.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>
/**
 * One quarter turn of one face.
 *
 * Attributes
 * ----------
 * name: const char*
 *     Notation for the move, e.g. "R" or "U'".
 * cycle: int[4]
 *     The 4 slots this move permutes, in order: cycle[i] -> cycle[i + 1].
 * twist: int[4]
 *     Orientation delta (mod 3) applied to the piece leaving cycle[i].
 */
struct Move {
  const char *name;
  int cycle[4];
  int twist[4];
};

// Y axis, no orientation change.
inline constexpr Move MOVE_U = {"U", {2, 3, 1, 0}, {0, 0, 0, 0}};
inline constexpr Move MOVE_Up = {"U'", {2, 0, 1, 3}, {0, 0, 0, 0}};
inline constexpr Move MOVE_D = {"D", {4, 5, 7, 6}, {0, 0, 0, 0}};
inline constexpr Move MOVE_Dp = {"D'", {4, 6, 7, 5}, {0, 0, 0, 0}};

// X axis.
inline constexpr Move MOVE_R = {"R", {1, 3, 7, 5}, {2, 1, 2, 1}};
inline constexpr Move MOVE_Rp = {"R'", {1, 5, 7, 3}, {2, 1, 2, 1}};
inline constexpr Move MOVE_L = {"L", {0, 4, 6, 2}, {1, 2, 1, 2}};
inline constexpr Move MOVE_Lp = {"L'", {0, 2, 6, 4}, {1, 2, 1, 2}};

// Z axis.
inline constexpr Move MOVE_F = {"F", {0, 1, 5, 4}, {2, 1, 2, 1}};
inline constexpr Move MOVE_Fp = {"F'", {0, 4, 5, 1}, {2, 1, 2, 1}};
inline constexpr Move MOVE_B = {"B", {2, 6, 7, 3}, {1, 2, 1, 2}};
inline constexpr Move MOVE_Bp = {"B'", {2, 3, 7, 6}, {1, 2, 1, 2}};

// Inverse pairs are adjacent: (U,U') (D,D') (R,R') (L,L') (F,F') (B,B').
inline constexpr Move ALL_MOVES[12] = {
    MOVE_U, MOVE_Up, MOVE_D, MOVE_Dp, MOVE_R, MOVE_Rp,
    MOVE_L, MOVE_Lp, MOVE_F, MOVE_Fp, MOVE_B, MOVE_Bp,
};

inline constexpr Move THREE_ADJACENT_MOVES[6] = {
    MOVE_R, MOVE_Rp, MOVE_U, MOVE_Up, MOVE_F, MOVE_Fp,
};

/**
 * Index, within a move array that pairs each move with its inverse at
 * adjacent indices (the convention both ALL_MOVES and
 * THREE_ADJACENT_MOVES follow), of the inverse of the move at index i.
 *
 * Parameters
 * ----------
 * i: int
 *     Index of the move whose inverse is wanted.
 *
 * Returns
 * -------
 * int
 *     Index of that move's inverse in the same array.
 */
inline constexpr int inverse_index(int i) { return i ^ 1; }

/**
 * Apply a slot cycle and twist deltas to a state.
 *
 * Parameters
 * ----------
 * s: State
 *     Starting state.
 * cycle: const int[4]
 *     The 4 slots to permute, in order.
 * twist: const int[4]
 *     Orientation delta (mod 3) applied to the piece leaving cycle[i].
 *
 * Returns
 * -------
 * State
 *     The state after the cycle and twists are applied.
 */
inline State transition(State s, const int cycle[4], const int twist[4]) {
  State out = s;
  for (int i = 0; i < 4; ++i) {
    uint8_t piece = s.slots[cycle[i]];
    uint8_t id = piece >> 2;
    uint8_t ori = ((piece & 0x03) + twist[i]) % 3;
    out.slots[cycle[(i + 1) & 3]] = (id << 2) | ori;
  }
  return out;
}

/**
 * Apply a named move to a state.
 *
 * Parameters
 * ----------
 * s: State
 *     Starting state.
 * m: const Move&
 *     Move to apply.
 *
 * Returns
 * -------
 * State
 *     The state after the move.
 */
inline State transition(State s, const Move &m) {
  return transition(s, m.cycle, m.twist);
}

/**
 * Apply a named move to a packed state.
 *
 * Parameters
 * ----------
 * s: uint64_t
 *     Starting state, packed.
 * m: const Move&
 *     Move to apply.
 *
 * Returns
 * -------
 * uint64_t
 *     The state after the move, packed.
 */
inline uint64_t apply_move(uint64_t s, const Move &m) {
  return transition(State(s), m).full_state;
}

/**
 * The 24 whole-cube rotations of SOLVED_STATE, found by composing existing
 * moves: turning a pair of opposite faces together in matching senses
 * (e.g. R then L') is geometrically a whole-cube rotation, since no
 * middle layer is left standing still to block it.
 *
 * A 2x2 has no fixed centres, so none of these 24 states is more
 * "correct" than another -- every solver treats all of them as solved.
 *
 * Returns
 * -------
 * const std::vector<uint64_t>&
 *     Exactly 24 states: the cube's chiral rotation group.
 */
inline const std::vector<uint64_t> &solved_orbit() {
  static const std::vector<uint64_t> orbit = [] {
    using Rotation = uint64_t (*)(uint64_t);
    const Rotation rotations[4] = {
        [](uint64_t s) { return apply_move(apply_move(s, MOVE_R), MOVE_Lp); },
        [](uint64_t s) { return apply_move(apply_move(s, MOVE_Rp), MOVE_L); },
        [](uint64_t s) { return apply_move(apply_move(s, MOVE_U), MOVE_Dp); },
        [](uint64_t s) { return apply_move(apply_move(s, MOVE_Up), MOVE_D); },
    };
    std::vector<uint64_t> result{SOLVED_STATE};
    for (std::size_t i = 0; i < result.size(); ++i) {
      for (Rotation rotate : rotations) {
        uint64_t next = rotate(result[i]);
        if (std::find(result.begin(), result.end(), next) == result.end())
          result.push_back(next);
      }
    }
    return result;
  }();
  return orbit;
}

/**
 * The state(s) a search should treat as having reached `goal`: all 24
 * whole-cube rotations when `goal` is SOLVED_STATE, or just `goal` itself
 * otherwise. Lets a multi-source search (e.g. bidirectional BFS's
 * backward half) seed every acceptable root at once.
 *
 * Parameters
 * ----------
 * goal: uint64_t
 *     Target state (default is SOLVED_STATE).
 *
 * Returns
 * -------
 * std::vector<uint64_t>
 *     States that count as the goal.
 */
inline std::vector<uint64_t> goal_states(uint64_t goal = SOLVED_STATE) {
  if (goal == SOLVED_STATE)
    return solved_orbit();
  return {goal};
}

/**
 * Whether `s` counts as having reached `goal` -- see goal_states().
 *
 * Parameters
 * ----------
 * s: uint64_t
 *     State to test.
 * goal: uint64_t
 *     Target state (default is SOLVED_STATE).
 *
 * Returns
 * -------
 * bool
 *     True if `s` is `goal` itself, or (when goal is SOLVED_STATE) any of
 *     its 24 whole-cube rotations.
 */
inline bool is_goal(uint64_t s, uint64_t goal = SOLVED_STATE) {
  if (goal != SOLVED_STATE)
    return s == goal;
  const std::vector<uint64_t> &orbit = solved_orbit();
  return std::find(orbit.begin(), orbit.end(), s) != orbit.end();
}
