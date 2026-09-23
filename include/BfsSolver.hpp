#pragma once

#include <deque>
#include <unordered_set>

#include "Solve.hpp"

class BfsSolver : public CubeSolver {
public:
  using CubeSolver::CubeSolver;

  SearchResult solve(State start, State goal = SOLVED_STATE) const override {
    SearchResult result;
    if (evaluate_state(start, goal)) {
      result.found = true;
      return result;
    }

    // 1. Adicionar estado na fila
    std::deque<Node> fila{Node{start, 0, start, -1}};

    ParentLinks parentOf;
    std::unordered_set<State> closed;

    // 2. Enquanto a fila não estiver vazia
    while (!fila.empty()) {
      // 2.1 Remover o estado mais antigo da fila
      Node current = fila.front();
      fila.pop_front();
      if (!closed.insert(current.state).second)
        continue; // já consolidado
      parentOf[current.state] = Link{current.parent, current.moveIndex};
      ++result.expanded;

      // 2.2 Avaliar estado
      if (evaluate_state(current.state, goal)) {
        // 2.2.1 SE estado final -> mostrar solução e encerrar
        result.found = true;
        result.moves = trace_path(parentOf, moves_, start, current.state);
        return result;
      }

      // 2.3 Adicionar estados seguintes no fim da fila
      for (const Node &next : generate_successors(current, moves_)) {
        if (!closed.count(next.state))
          fila.push_back(next);
      }
    }

    // 3. Retornar "Sem solução"
    return result;
  }
};
