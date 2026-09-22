#pragma once

#include <algorithm>
#include <deque>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Solve.hpp"

class AStarSolver : public CubeSolver {
public:
  using CubeSolver::CubeSolver;

  SearchResult solve(uint64_t start,
                     uint64_t goal = SOLVED_STATE) const override {
    SearchResult result;
    if (is_goal(start, goal)) {
      result.found = true;
      return result;
    }

    PatternDatabase heuristic(goal, moves_);

    std::priority_queue<Node, std::vector<Node>, NodeOrder> fila(
        NodeOrder{&heuristic});
    fila.push(Node{start, 0, start, -1});

    ParentLinks parentOf;
    std::unordered_set<uint64_t> closed;

    // 2. Enquanto a fila não estiver vazia
    while (!fila.empty()) {
      // 2.1 Remover o estado com o menor f = depth + h
      Node current = fila.top();
      fila.pop();
      if (!closed.insert(current.state).second)
        continue; // já consolidado
      parentOf[current.state] = Link{current.parent, current.moveIndex};
      ++result.expanded;

      // 2.2 Avaliar estado
      if (is_goal(current.state, goal)) {
        // 2.2.1 SE estado final -> mostrar solução e encerrar
        result.found = true;
        result.moves = trace_path(parentOf, moves_, start, current.state);
        return result;
      }

      // 2.3 Adicionar estados seguintes na fila
      for (const Node &next : generate_successors(current, moves_)) {
        if (!closed.count(next.state))
          fila.push(next);
      }
    }

    // 3. Retornar "Sem solução"
    return result;
  }

private:
  class PatternDatabase {
  public:
    /**
     * Parâmetros
     * ----------
     * goal: uint64_t
     *     Estado alvo (SOLVED_STATE significa qualquer uma de suas 24
     *     rotações -- ver goal_states()).
     * moves: const std::vector<Move>&
     *     Movimentos do solucionador; cada um custa 1.
     */
    PatternDatabase(uint64_t goal, const std::vector<Move> &moves)
        : orientation_(build(goal, moves, ORIENTATION_MASK)),
          permutation_(build(goal, moves, PERMUTATION_MASK)) {}
    int operator()(uint64_t s) const {
      auto ori = orientation_.find(s & ORIENTATION_MASK);
      auto perm = permutation_.find(s & PERMUTATION_MASK);
      if (ori == orientation_.end() || perm == permutation_.end())
        return UNREACHABLE;
      return std::max(ori->second, perm->second);
    }

    static constexpr int UNREACHABLE = 1 << 20;

  private:
    typedef std::unordered_map<uint64_t, int> Table;

    static constexpr uint64_t ORIENTATION_MASK = 0x0303030303030303ULL;
    static constexpr uint64_t PERMUTATION_MASK = 0xFCFCFCFCFCFCFCFCULL;
    static Table build(uint64_t goal, const std::vector<Move> &moves,
                       uint64_t mask) {
      Table distance;
      std::deque<uint64_t> frontier;
      for (uint64_t g : goal_states(goal)) {
        if (distance.emplace(g & mask, 0).second)
          frontier.push_back(g & mask);
      }
      while (!frontier.empty()) {
        uint64_t key = frontier.front();
        frontier.pop_front();
        int nextDistance = distance.at(key) + 1;
        for (const Move &m : moves) {
          uint64_t prev =
              apply_move(apply_move(apply_move(key, m), m), m) & mask;
          if (distance.emplace(prev, nextDistance).second)
            frontier.push_back(prev);
        }
      }
      return distance;
    }

    Table orientation_;
    Table permutation_;
  };

  struct NodeOrder {
    const PatternDatabase *heuristic;
    bool operator()(const Node &a, const Node &b) const {
      return a.depth + (*heuristic)(a.state) > b.depth + (*heuristic)(b.state);
    }
  };
};
