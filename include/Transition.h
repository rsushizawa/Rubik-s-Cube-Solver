#pragma once

#include <cstdint>
#include "State.h"

// ===========================================================================
//  Corner model (2x2x2)
// ===========================================================================
//  State packs 8 slots into a uint64_t, one byte per slot:
//
//      byte = (id << 2) | ori
//        id  : 0..7  which corner cubie currently sits in this slot
//        ori : 0..2  twist of that cubie about its body diagonal
//
//  Slots are fixed points in space. Index bits: bit0 = X, bit1 = Z, bit2 = Y
//  (0 => +, 1 => -):
//
//      0:(-X,+Y,+Z)  1:(+X,+Y,+Z)  2:(-X,+Y,-Z)  3:(+X,+Y,-Z)
//      4:(-X,-Y,+Z)  5:(+X,-Y,+Z)  6:(-X,-Y,-Z)  7:(+X,-Y,-Z)
//
//  Solved: cubie i in slot i, ori 0  ->  0x1C1814100C080400
//
// ===========================================================================
//  Moves
// ===========================================================================
//  A quarter turn is a 4-cycle of slots plus a per-step twist delta (mod 3):
//
//      piece at cycle[i]  moves to  cycle[(i + 1) % 4]
//      new_ori = (old_ori + twist[i]) % 3
//
//  U, R, F are  -90 deg rotations about +Y, +X, +Z.
//  D, L, B are  +90 deg rotations about the same axes (still clockwise when
//  viewed from outside their own face).  Primes are the inverses.
//
//  U / D never change orientation (ori is measured against the U/D sticker).
//  X-axis turns (R / L) twist by {2,1,2,1} / {1,2,1,2} along their cycle.
//  Z-axis turns (F / B) twist by {2,1,2,1} / {1,2,1,2} along their cycle.
//
//  All tables were derived by tracking the U/D sticker of each moving corner
//  through the rotation, and cross-checked against  m^4 = e,  m * m' = e,
//  (R U R' U')^6 = e.

struct Move {
  const char* name;
  int cycle[4];
  int twist[4];
};

// -- U / D : Y axis (no orientation change) ------------------------------
inline constexpr Move MOVE_U  = { "U",  {2, 3, 1, 0}, {0, 0, 0, 0} };
inline constexpr Move MOVE_Up = { "U'", {2, 0, 1, 3}, {0, 0, 0, 0} };
inline constexpr Move MOVE_D  = { "D",  {4, 5, 7, 6}, {0, 0, 0, 0} };
inline constexpr Move MOVE_Dp = { "D'", {4, 6, 7, 5}, {0, 0, 0, 0} };

// -- R / L : X axis -----------------------------------------------------
inline constexpr Move MOVE_R  = { "R",  {1, 3, 7, 5}, {2, 1, 2, 1} };
inline constexpr Move MOVE_Rp = { "R'", {1, 5, 7, 3}, {2, 1, 2, 1} };
inline constexpr Move MOVE_L  = { "L",  {0, 4, 6, 2}, {1, 2, 1, 2} };
inline constexpr Move MOVE_Lp = { "L'", {0, 2, 6, 4}, {1, 2, 1, 2} };

// -- F / B : Z axis -----------------------------------------------------
inline constexpr Move MOVE_F  = { "F",  {0, 1, 5, 4}, {2, 1, 2, 1} };
inline constexpr Move MOVE_Fp = { "F'", {0, 4, 5, 1}, {2, 1, 2, 1} };
inline constexpr Move MOVE_B  = { "B",  {2, 6, 7, 3}, {1, 2, 1, 2} };
inline constexpr Move MOVE_Bp = { "B'", {2, 3, 7, 6}, {1, 2, 1, 2} };

// Inverse pairs are adjacent: (U,U') (D,D') (R,R') (L,L') (F,F') (B,B').
inline constexpr Move ALL_MOVES[12] = {
  MOVE_U, MOVE_Up, MOVE_D, MOVE_Dp,
  MOVE_R, MOVE_Rp, MOVE_L, MOVE_Lp,
  MOVE_F, MOVE_Fp, MOVE_B, MOVE_Bp,
};

// -- Application ---------------------------------------------------------

inline State transition(State s, const int cycle[4], const int twist[4]) {
  State out = s;
  for (int i = 0; i < 4; ++i) {
    uint8_t piece = s.slots[cycle[i]];
    uint8_t id  = piece >> 2;
    uint8_t ori = ((piece & 0x03) + twist[i]) % 3;
    out.slots[cycle[(i + 1) & 3]] = (id << 2) | ori;
  }
  return out;
}

inline State transition(State s, const Move& m) {
  return transition(s, m.cycle, m.twist);
}

inline uint64_t apply_move(uint64_t s, const Move& m) {
  return transition(State(s), m).full_state;
}
