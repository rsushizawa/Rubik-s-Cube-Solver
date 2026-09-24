#pragma once

#include <algorithm>
#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Solve.hpp"

class AStarSolver : public CubeSolver {
public:
  using CubeSolver::CubeSolver;

  SearchResult solve(State start, State goal = SOLVED_STATE) const override {
    SearchResult result;
    if (evaluate_state(start, goal)) {
      result.found = true;
      return result;
    }

    std::optional<PatternDatabase> otherGoal;
    const PatternDatabase &heuristic =
        goal == SOLVED_STATE ? solved_table() : otherGoal.emplace(goal, moves_);

    std::priority_queue<Node, std::vector<Node>, NodeOrder> fila(
        NodeOrder{&heuristic});
    fila.push(Node{start, 0, start, -1});

    ParentLinks parentOf;
    std::unordered_set<State> closed;

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
      if (evaluate_state(current.state, goal)) {
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

  /**
   * Monta as tabelas da heurística para SOLVED_STATE, se ainda não foram
   * montadas. Elas são montadas uma única vez e reaproveitadas por todo
   * solve() com esse objetivo; chamar isto antes da primeira busca tira o
   * custo da montagem do tempo de busca.
   *
   * Retorna
   * -------
   * double
   *     Quanto tempo a montagem levou, em milissegundos.
   */
  double prepare() const {
    solved_table();
    return buildMs_;
  }

private:
  class PatternDatabase {
  public:
    /**
     * Parâmetros
     * ----------
     * goal: State
     *     Estado alvo (SOLVED_STATE significa qualquer uma de suas 24
     *     rotações -- ver goal_states()).
     * moves: const std::vector<Move>&
     *     Movimentos do solucionador; cada um custa 1.
     */
    PatternDatabase(State goal, const std::vector<Move> &moves)
        : orientation_(build(goal, moves, ORIENTATION_MASK)),
          permutation_(build(goal, moves, PERMUTATION_MASK)) {}
    // h(s): o que exigir mais movimentos -- acertar só as orientações ou
    // só as posições.
    int operator()(State s) const {
      int orientationMoves = lookup(orientation_, s, ORIENTATION_MASK);
      int permutationMoves = lookup(permutation_, s, PERMUTATION_MASK);
      return std::max(orientationMoves, permutationMoves);
    }

    static constexpr int UNREACHABLE = 1 << 20;

  private:
    typedef std::unordered_map<State, int> Table;

    static constexpr uint64_t ORIENTATION_MASK = 0x0303030303030303ULL;
    static constexpr uint64_t PERMUTATION_MASK = 0xFCFCFCFCFCFCFCFCULL;

    static State project(State s, uint64_t mask) {
      return State(s.full_state & mask);
    }

    static int lookup(const Table &table, State s, uint64_t mask) {
      auto entry = table.find(project(s, mask));
      return entry == table.end() ? UNREACHABLE : entry->second;
    }

    static Table build(State goal, const std::vector<Move> &moves,
                       uint64_t mask) {
      Table distance;
      std::deque<State> frontier;
      for (State g : goal_states(goal)) {
        if (distance.emplace(project(g, mask), 0).second)
          frontier.push_back(project(g, mask));
      }
      while (!frontier.empty()) {
        State key = frontier.front();
        frontier.pop_front();
        int nextDistance = distance.at(key) + 1;
        for (const Move &m : moves) {
          State prev =
              project(transition(transition(transition(key, m), m), m), mask);
          if (distance.emplace(prev, nextDistance).second)
            frontier.push_back(prev);
        }
      }
      return distance;
    }

    Table orientation_;
    Table permutation_;
  };

  mutable std::once_flag built_;
  mutable std::unique_ptr<PatternDatabase> solvedTable_;
  mutable double buildMs_ = 0.0;

  const PatternDatabase &solved_table() const {
    std::call_once(built_, [this] {
      auto t0 = std::chrono::steady_clock::now();
      solvedTable_ = std::make_unique<PatternDatabase>(SOLVED_STATE, moves_);
      buildMs_ = std::chrono::duration<double, std::milli>(
                     std::chrono::steady_clock::now() - t0)
                     .count();
    });
    return *solvedTable_;
  }

  struct NodeOrder {
    const PatternDatabase *heuristic;

    // Função de avaliação do A*: f(n) = g(n) + h(n),
    // g(n) = quantidade de movimentos já feitos
    // h(n) = estimativa dos que faltam.
    int f(const Node &n) const { return n.depth + (*heuristic)(n.state); }

    bool operator()(const Node &a, const Node &b) const { return f(a) > f(b); }
  };
};
