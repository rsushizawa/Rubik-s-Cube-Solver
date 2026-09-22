#pragma once

#include <algorithm>
#include <cstdint>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "State.hpp"
#include "Transition.hpp"

/**
 * Parâmetros
 * ----------
 * moves: const std::vector<Move>&
 *     Movimentos dos quais o embaralhamento pode sortear, ex.: ALL_MOVES.
 * length: int
 *     Quantidade de movimentos a gerar.
 * seed: uint32_t
 *     Semente do gerador aleatório (padrão é não-determinístico).
 *
 * Retorna
 * -------
 * std::vector<std::string>
 *     `length` nomes de moviment
 */
inline std::vector<std::string>
scramble(const std::vector<Move> &moves, int length,
         uint32_t seed = std::random_device{}()) {

  std::mt19937 rng(seed);
  std::uniform_int_distribution<int> pick(0, (int)moves.size() - 1);
  std::vector<std::string> result;
  int last = -1;
  while ((int)result.size() < length) {
    int chosen = pick(rng);
    if (last >= 0 && moves[chosen].name[0] == moves[last].name[0])
      continue;
    result.push_back(moves[chosen].name);
    last = chosen;
  }
  return result;
}

/**
 * Atributos
 * ----------
 * found: bool
 *     Se um caminho de start até goal foi encontrado.
 * moves: std::vector<std::string>
 *     Nomes dos movimentos, aplicados da esquerda para a direita:
 *     start --moves--> goal.
 * expanded: std::uint64_t
 *     Nós expandidos durante a busca (estatística, não usada para a
 *     corretude do resultado).
 */
struct SearchResult {
  bool found = false;
  std::vector<std::string> moves;
  std::uint64_t expanded = 0;

  /**
   * Renderiza a lista de movimentos como uma única string separada por
   * espaços.
   *
   * Retorna
   * -------
   * std::string
   *     Os movimentos em ordem, ex.: "R U R' U'".
   */
  std::string notation() const {
    std::string text;
    for (std::size_t i = 0; i < moves.size(); ++i) {
      if (i)
        text += ' ';
      text += moves[i];
    }
    return text;
  }
};

/**
 * Parâmetros
 * ----------
 * moves: const std::vector<Move>&
 *     O conjunto de movimentos do qual os nomes em result.moves foram tirados.
 * start: uint64_t
 *     Estado a partir do qual o solucionador foi chamado.
 * result: const SearchResult&
 *     O valor retornado por ele.
 * goal: uint64_t
 *     Alvo passado para solve() (padrão SOLVED_STATE).
 *
 * Retorna
 * -------
 * bool
 *     Verdadeiro se reproduzir result.moves a partir de start alcança
 *     goal (ou, quando goal é SOLVED_STATE, qualquer uma de suas 24
 *     rotações -- ver is_goal()).
 */
inline bool verify_solution(const std::vector<Move> &moves, uint64_t start,
                            const SearchResult &result,
                            uint64_t goal = SOLVED_STATE) {
  if (!result.found)
    return false;
  uint64_t state = start;
  for (const std::string &name : result.moves) {
    bool applied = false;
    for (const Move &m : moves) {
      if (name == m.name) {
        state = apply_move(state, m);
        applied = true;
        break;
      }
    }
    if (!applied)
      return false;
  }
  return is_goal(state, goal);
}

struct Node {
  uint64_t state;
  int depth;
  uint64_t parent;
  int moveIndex;
};

struct Link {
  uint64_t parent;
  int moveIndex;
};

typedef std::unordered_map<uint64_t, Link> ParentLinks;

inline std::vector<std::string> trace_path(const ParentLinks &parentOf,
                                           const std::vector<Move> &moves,
                                           uint64_t start, uint64_t goal) {
  std::vector<std::string> path;
  for (uint64_t state = goal; state != start;) {
    const Link &link = parentOf.at(state);
    path.push_back(moves[link.moveIndex].name);
    state = link.parent;
  }
  std::reverse(path.begin(), path.end());
  return path;
}

/**
 * Função Sucessora: todo estado a um movimento de distância de
 * `node.state`.
 *
 * Parâmetros
 * ----------
 * node: const Node&
 *     O estado do qual gerar sucessores.
 * moves: const std::vector<Move>&
 *     Movimentos pelos quais um sucessor pode ser alcançado.
 *
 * Retorna
 * -------
 * std::vector<Node>
 *     Um Node por movimento, na mesma ordem de `moves`.
 */
inline std::vector<Node> generate_successors(const Node &node,
                                             const std::vector<Move> &moves) {
  std::vector<Node> successors;
  successors.reserve(moves.size());
  for (std::size_t i = 0; i < moves.size(); ++i) {
    uint64_t next = apply_move(node.state, moves[i]);
    successors.push_back(Node{next, node.depth + 1, node.state, (int)i});
  }
  return successors;
}

class CubeSolver {
public:
  /**
   * Parâmetros
   * ----------
   * moves: std::vector<Move>
   *     Movimentos que a busca deste solucionador pode usar.
   */
  explicit CubeSolver(std::vector<Move> moves) : moves_(std::move(moves)) {}
  virtual ~CubeSolver() = default;

  /**
   * Busca um caminho de `start` até `goal`.
   *
   * Parâmetros
   * ----------
   * start: uint64_t
   *     Estado inicial.
   * goal: uint64_t
   *     Estado alvo (padrão é SOLVED_STATE).
   *
   * Retorna
   * -------
   * SearchResult
   *     O resultado da busca; ver SearchResult.
   */
  virtual SearchResult solve(uint64_t start,
                             uint64_t goal = SOLVED_STATE) const = 0;

  const std::vector<Move> &moves() const { return moves_; }

protected:
  std::vector<Move> moves_;
};
