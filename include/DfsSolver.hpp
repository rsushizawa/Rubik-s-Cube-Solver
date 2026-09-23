#pragma once

#include <stack>
#include <unordered_map>

#include "Solve.hpp"

class DfsSolver : public CubeSolver {
public:
  /**
   * Parâmetros
   * ----------
   * moves: std::vector<Move>
   *     Movimentos que a busca pode usar.
   * limit: int
   *     Profundidade máxima explorada. 14 é o número de Deus do 2x2 em
   *     giros de 90 graus: com os 12 movimentos, todo estado tem solução
   *     dentro desse limite.
   */
  explicit DfsSolver(std::vector<Move> moves, int limit = 14)
      : CubeSolver(std::move(moves)), limit_(limit) {}

  int limit() const { return limit_; }
  void set_limit(int limit) { limit_ = limit; }

  SearchResult solve(State start, State goal = SOLVED_STATE) const override {
    return solve_limited(start, goal, limit_);
  }

  /**
   * A busca em profundidade limitada em si, com o limite passado
   * explicitamente -- usada por solve() e pelo IddfsSolver, que a chama
   * com limites crescentes.
   *
   * Parâmetros
   * ----------
   * start: State
   *     Estado inicial.
   * goal: State
   *     Estado alvo.
   * limit: int
   *     Profundidade máxima explorada.
   *
   * Retorna
   * -------
   * SearchResult
   *     A solução, se existir uma com até `limit` movimentos.
   */
  SearchResult solve_limited(State start, State goal, int limit) const {
    SearchResult result;
    if (evaluate_state(start, goal)) {
      result.found = true;
      return result;
    }

    // 1. Adicionar estado na pilha
    std::stack<Node> pilha;
    pilha.push(Node{start, 0, start, -1});

    ParentLinks parentOf;
    std::unordered_map<State, int> bestDepth;

    // 2. Enquanto a pilha não estiver vazia
    while (!pilha.empty()) {
      // 2.1 Remover o estado mais recente da pilha
      Node current = pilha.top();
      pilha.pop();
      auto seen = bestDepth.find(current.state);
      if (seen != bestDepth.end() && seen->second <= current.depth)
        continue; // já expandido por um caminho tão curto quanto este
      bestDepth[current.state] = current.depth;
      parentOf[current.state] = Link{current.parent, current.moveIndex};
      ++result.expanded;

      // 2.2 Avaliar estado
      if (evaluate_state(current.state, goal)) {
        // 2.2.1 SE estado final -> mostrar solução e encerrar
        result.found = true;
        result.moves = trace_path(parentOf, moves_, start, current.state);
        return result;
      }

      // 2.3 Adicionar estados seguintes no topo da pilha, até o limite
      if (current.depth >= limit)
        continue;
      for (const Node &next : generate_successors(current, moves_)) {
        auto best = bestDepth.find(next.state);
        if (best == bestDepth.end() || next.depth < best->second)
          pilha.push(next);
      }
    }

    // 3. Retornar "Sem solução"
    return result;
  }

private:
  int limit_;
};
