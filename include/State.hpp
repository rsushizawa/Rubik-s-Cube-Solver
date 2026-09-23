#pragma once
#include <cstdint>
#include <functional>

struct State {
  union {
    uint64_t full_state;
    uint8_t slots[8];
  };

  constexpr State() : full_state(0) {}
  constexpr State(uint64_t val) : full_state(val) {}
};

inline bool operator==(const State &a, const State &b) {
  return a.full_state == b.full_state;
}

inline bool operator!=(const State &a, const State &b) {
  return a.full_state != b.full_state;
}

template <> struct std::hash<State> {
  std::size_t operator()(const State &s) const {
    return std::hash<uint64_t>()(s.full_state);
  }
};

inline constexpr State SOLVED_STATE{0x1C1814100C080400ULL};
