#pragma once

#include "State.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>
/**
 * Um giro de 90 graus de uma face.
 *
 * Atributos
 * ----------
 * name: const char*
 *     Notação do movimento, ex.: "R" ou "U'".
 * cycle: int[4]
 *     As 4 posições que este movimento permuta, em ordem: cycle[i] -> cycle[i +
 * 1]. twist: int[4] Delta de orientação (mod 3) aplicado à peça que sai de
 * cycle[i].
 */
struct Move {
  const char *name;
  int cycle[4];
  int twist[4];
};

// Eixo Y, sem mudança de orientação.
inline constexpr Move MOVE_U = {"U", {2, 3, 1, 0}, {0, 0, 0, 0}};
inline constexpr Move MOVE_Up = {"U'", {2, 0, 1, 3}, {0, 0, 0, 0}};
inline constexpr Move MOVE_D = {"D", {4, 5, 7, 6}, {0, 0, 0, 0}};
inline constexpr Move MOVE_Dp = {"D'", {4, 6, 7, 5}, {0, 0, 0, 0}};

// Eixo X.
inline constexpr Move MOVE_R = {"R", {1, 3, 7, 5}, {2, 1, 2, 1}};
inline constexpr Move MOVE_Rp = {"R'", {1, 5, 7, 3}, {2, 1, 2, 1}};
inline constexpr Move MOVE_L = {"L", {0, 4, 6, 2}, {1, 2, 1, 2}};
inline constexpr Move MOVE_Lp = {"L'", {0, 2, 6, 4}, {1, 2, 1, 2}};

// Eixo Z.
inline constexpr Move MOVE_F = {"F", {0, 1, 5, 4}, {2, 1, 2, 1}};
inline constexpr Move MOVE_Fp = {"F'", {0, 4, 5, 1}, {2, 1, 2, 1}};
inline constexpr Move MOVE_B = {"B", {2, 6, 7, 3}, {1, 2, 1, 2}};
inline constexpr Move MOVE_Bp = {"B'", {2, 3, 7, 6}, {1, 2, 1, 2}};

// Pares inversos ficam adjacentes: (U,U') (D,D') (R,R') (L,L') (F,F') (B,B').
inline constexpr Move ALL_MOVES[12] = {
    MOVE_U, MOVE_Up, MOVE_D, MOVE_Dp, MOVE_R, MOVE_Rp,
    MOVE_L, MOVE_Lp, MOVE_F, MOVE_Fp, MOVE_B, MOVE_Bp,
};

inline constexpr Move THREE_ADJACENT_MOVES[6] = {
    MOVE_R, MOVE_Rp, MOVE_U, MOVE_Up, MOVE_F, MOVE_Fp,
};
/**
 * Aplica um ciclo de posições e os deltas de torção a um estado.
 *
 * Parâmetros
 * ----------
 * s: State
 *     Estado inicial.
 * cycle: const int[4]
 *     As 4 posições a permutar, em ordem.
 * twist: const int[4]
 *     Delta de orientação (mod 3) aplicado à peça que sai de cycle[i].
 *
 * Retorna
 * -------
 * State
 *     O estado após o ciclo e as torções serem aplicados.
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

inline State transition(State s, const Move &m) {
  return transition(s, m.cycle, m.twist);
}
inline uint64_t apply_move(uint64_t s, const Move &m) {
  return transition(State(s), m).full_state;
}

inline const std::vector<uint64_t> &solved_orbit() {
  static const std::vector<uint64_t> orbit = [] {
    typedef uint64_t (*Rotation)(uint64_t);
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

inline std::vector<uint64_t> goal_states(uint64_t goal = SOLVED_STATE) {
  if (goal == SOLVED_STATE)
    return solved_orbit();
  return {goal};
}

inline bool is_goal(uint64_t s, uint64_t goal = SOLVED_STATE) {
  if (goal != SOLVED_STATE)
    return s == goal;
  const std::vector<uint64_t> &orbit = solved_orbit();
  return std::find(orbit.begin(), orbit.end(), s) != orbit.end();
}
