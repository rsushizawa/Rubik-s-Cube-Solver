#pragma once
#include <cstdint>

using namespace std;

struct State {
  union {
    uint64_t full_state;
    uint8_t slots[8];
  };

  State(uint64_t val){
    full_state = val;
  }
};

inline constexpr uint64_t SOLVED_STATE = 0x1C1814100C080400ULL;
