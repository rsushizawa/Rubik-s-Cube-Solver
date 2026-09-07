#include <cstdint>
#include "State.h"

using namespace std;

// U
const int U_CYCLE[4] = {2, 3, 1, 0};
const int U_TWIST[4] = {0, 0, 0, 0};

// R
const int R_CYCLE[4] = {1, 3, 7, 5};
const int R_TWIST[4] = {2, 1, 2, 1};

// F
const int F_CYCLE[4] = {0, 1, 5, 4};
const int F_TWIST[4] = {2, 1, 2, 1};

State transition(State curr_state, const int cycle[4], const int twist[4]){
  State new_state = curr_state;

  for(int i = 0; i < 4; i++){
    int origin_slot = cycle[i];
    int dest_slot = cycle[(i + 1) % 4];

    uint8_t piece = curr_state.slots[origin_slot];

    uint8_t id = piece >> 2;
    uint8_t ori = piece & 0x03;

    uint8_t new_ori = (ori + twist[i]) % 3;

    new_state.slots[dest_slot] = (id << 2) | new_ori;
  }

  return new_state;
}

