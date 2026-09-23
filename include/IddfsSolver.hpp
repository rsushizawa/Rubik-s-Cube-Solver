#pragma once

#include "DfsSolver.hpp"

class IddfsSolver : public CubeSolver {
public:
  /**
   * Parâmetros
   * ----------
   * moves: std::vector<Move>
   *     Movimentos que a busca pode usar.
   * maxLimit: int
   *     Último limite tentado. 14 é o número de Deus do 2x2 em giros de
   *     90 graus: com os 12 movimentos, a busca sempre encontra solução.
   */
  explicit IddfsSolver(std::vector<Move> moves, int maxLimit = 14)
      : CubeSolver(moves), dfs_(std::move(moves)), maxLimit_(maxLimit) {}

  SearchResult solve(State start, State goal = SOLVED_STATE) const override {
    SearchResult total;
    for (int limit = 0; limit <= maxLimit_; ++limit) {
      SearchResult round = dfs_.solve_limited(start, goal, limit);
      total.expanded += round.expanded;
      if (round.found) {
        total.found = true;
        total.moves = round.moves;
        return total;
      }
    }
    return total;
  }

private:
  DfsSolver dfs_;
  int maxLimit_;
};
