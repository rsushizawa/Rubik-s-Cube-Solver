#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <vector>

#include "Solve.h"

/**
 * Solucionador de duas fases, no estilo Kociemba/Thistlethwaite.
 *
 * A fase 1 leva a orientação a zero em todas as posições, usando
 * qualquer um dos 12 movimentos. A fase 2 então leva a permutação ao
 * estado resolvido, usando apenas movimentos que comprovadamente nunca
 * voltam a perturbar a orientação: giros simples de U/D (a torção é
 * sempre 0) mais giros *duplos* de qualquer outra face -- verificado à
 * parte que aplicar qualquer movimento simples duas vezes sempre
 * devolve a mudança líquida de orientação a zero (o padrão de torção de
 * cada face soma um múltiplo de 3 ao longo de dois passos consecutivos
 * do ciclo), e que este conjunto de 8 geradores {U, U', D, D', R2, L2,
 * F2, B2} alcança as 40.320 permutações sem jamais quebrar a orientação
 * pelo caminho.
 *
 * As duas fases buscam em um pequeno espaço *projetado* (2.187 chaves
 * de orientação, 40.320 chaves de permutação), em vez do grafo completo
 * de 88 milhões de estados, então nenhuma das duas fases chega perto da
 * profundidade em que AStarSolver::heuristic() satura. O custo: isto não é
 * globalmente ótimo. Uma solução realmente mais curta pode intercalar
 * movimentos que corrigem orientação e permutação de formas que a fase
 * 2 não consegue alcançar (ela fica restrita a não usar R, L, F, B como
 * giros simples), então o resultado concatenado costuma ser mais longo
 * que a solução mais curta de verdade -- a mesma concessão que todo
 * método prático de resolver o cubo (Kociemba, Thistlethwaite, CFOP)
 * faz.
 *
 * Só funciona com o objetivo canônico (qualquer uma das 24 rotações
 * resolvidas): a propriedade de preservar a orientação, descrita acima,
 * foi verificada especificamente contra SOLVED_STATE, não contra um
 * alvo arbitrário.
 *
 * As chaves de orientação e de permutação são ambas representadas como
 * `State` (o mesmo layout compactado de identidade/orientação por
 * posição de um estado completo do cubo), só que com um dos dois campos
 * fixado num valor de referência -- não um tipo inteiro compactado à
 * parte. A mesma representação em todo lugar, menos coisas para aprender
 * para ler este arquivo.
 */
class TwoPhaseSolver : public CubeSolver {
public:
  using CubeSolver::CubeSolver;

  /**
   * Busca um caminho de `start` até `goal`, passando pela fase 1
   * (orientação) e depois pela fase 2 (permutação).
   *
   * Parâmetros
   * ----------
   * start: uint64_t
   *     Estado inicial.
   * goal: uint64_t
   *     Precisa ser SOLVED_STATE; qualquer outro valor retorna "não
   *     encontrado".
   *
   * Retorna
   * -------
   * SearchResult
   *     Uma sequência de movimentos de `start` até um estado resolvido,
   *     se existir uma. Não necessariamente a mais curta -- ver o
   *     comentário da classe.
   */
  SearchResult solve(uint64_t start, uint64_t goal = SOLVED_STATE) const override {
    SearchResult result;
    if (goal != SOLVED_STATE) return result;   // fora de escopo, ver comentário da classe
    if (is_goal(start, goal)) { result.found = true; return result; }

    Search phase1 = solve_phase1(State(start));
    State afterPhase1 = State(start);
    for (const std::string& name : phase1.moves) afterPhase1 = transition(afterPhase1, move_by_name(name));

    Search phase2 = solve_phase2(afterPhase1);

    result.found = true;
    result.moves = phase1.moves;
    result.moves.insert(result.moves.end(), phase2.moves.begin(), phase2.moves.end());
    result.expanded = phase1.expanded + phase2.expanded;
    return result;
  }

private:
  struct Search { std::vector<std::string> moves; std::uint64_t expanded = 0; };

  static const Move& move_by_name(const std::string& name) {
    for (const Move& m : ALL_MOVES) if (name == m.name) return m;
    throw std::invalid_argument("TwoPhaseSolver: movimento desconhecido '" + name + "'");
  }

  // ---------------------------------------------------------- fase 1

  /**
   * Projeta um estado somente na orientação: cada byte de posição
   * mantém sua torção, com a identidade forçada a 0.
   */
  static State ori_key(State s) {
    State key(0);
    for (int slot = 0; slot < 8; ++slot) key.slots[slot] = s.slots[slot] & 0x03;
    return key;
  }

  /**
   * Aplica um movimento a uma chave de orientação. Isto é apenas
   * transition() -- com a identidade forçada a 0 em toda posição, a
   * lógica normal de ciclo+torção de transition() já faz exatamente o
   * necessário (move os pares (0, torção) ao longo do ciclo,
   * acumulando a torção, deixando a identidade em 0), sem nada extra.
   */
  static State apply_move_to_ori_key(State key, const Move& m) {
    return transition(key, m);
  }

  static bool is_ori_solved(State key) {
    return key.full_state == 0;
  }

  /**
   * Sequência de movimentos mais curta (entre os 12 movimentos) que
   * leva a orientação de `start` inteiramente a zero. Busca no espaço
   * projetado de 2.187 chaves de orientação, não no estado completo.
   */
  static Search solve_phase1(State start) {
    Search result;
    State startKey = ori_key(start);
    if (is_ori_solved(startKey)) return result;

    struct Link { State parent; int moveIndex; };
    std::unordered_map<State, Link> parentOf;
    std::deque<State> frontier{startKey};
    parentOf.emplace(startKey, Link{startKey, -1});

    while (!frontier.empty()) {
      State current = frontier.front(); frontier.pop_front();
      ++result.expanded;
      for (int i = 0; i < 12; ++i) {
        State next = apply_move_to_ori_key(current, ALL_MOVES[i]);
        if (!parentOf.emplace(next, Link{current, i}).second) continue;
        if (is_ori_solved(next)) {
          std::vector<std::string> path;
          for (State k = next; !(k == startKey); ) {
            const Link& link = parentOf.at(k);
            path.push_back(ALL_MOVES[link.moveIndex].name);
            k = link.parent;
          }
          std::reverse(path.begin(), path.end());
          result.moves = std::move(path);
          return result;
        }
        frontier.push_back(next);
      }
    }
    return result;   // inalcançável na prática: a fase 1 cobre as 2187 chaves
  }

  // ---------------------------------------------------------- fase 2

  /**
   * Projeta um estado somente na permutação: cada byte de posição
   * mantém sua identidade, com a orientação forçada a 0.
   */
  static State perm_key(State s) {
    State key(0);
    for (int slot = 0; slot < 8; ++slot) {
      uint8_t id = s.slots[slot] >> 2;
      key.slots[slot] = id << 2;
    }
    return key;
  }

  /**
   * Aplica um movimento a uma chave de permutação, ignorando a torção.
   * Não é possível reaproveitar transition() aqui como
   * apply_move_to_ori_key() faz -- transition() somaria a torção real
   * de cada movimento ao byte de orientação (sempre 0), que é
   * exatamente a informação que esta projeção precisa ignorar.
   */
  static State apply_move_to_perm_key(State key, const Move& m) {
    State out = key;
    for (int i = 0; i < 4; ++i) {
      uint8_t id = key.slots[m.cycle[i]] >> 2;
      out.slots[m.cycle[(i + 1) & 3]] = id << 2;   // a orientação continua 0
    }
    return out;
  }

  // Um gerador da fase 2 é 1 movimento real (U, U', D, D') ou 2 (R2,
  // L2, F2, B2 -- a mesma face aplicada duas vezes).
  struct Generator { const Move* steps[2]; int stepCount; };

  static const std::array<Generator, 8>& phase2_generators() {
    static const std::array<Generator, 8> gens = {{
      {{&MOVE_U, nullptr}, 1}, {{&MOVE_Up, nullptr}, 1},
      {{&MOVE_D, nullptr}, 1}, {{&MOVE_Dp, nullptr}, 1},
      {{&MOVE_R, &MOVE_R}, 2}, {{&MOVE_L, &MOVE_L}, 2},
      {{&MOVE_F, &MOVE_F}, 2}, {{&MOVE_B, &MOVE_B}, 2},
    }};
    return gens;
  }

  static State apply_generator_to_perm_key(State key, const Generator& g) {
    for (int i = 0; i < g.stepCount; ++i) key = apply_move_to_perm_key(key, *g.steps[i]);
    return key;
  }

  /**
   * Os 8, dentre os 24 estados de solved_orbit(), que já têm orientação
   * inteiramente zero -- as únicas permutações em que a fase 2 pode
   * parar, já que a fase 1 garantiu que a orientação é uma dessas 8.
   */
  static std::vector<State> compute_phase2_targets() {
    std::vector<State> result;
    for (uint64_t s : solved_orbit()) {
      if (is_ori_solved(ori_key(State(s)))) result.push_back(perm_key(State(s)));
    }
    return result;
  }

  static const std::vector<State>& phase2_targets() {
    static const std::vector<State> targets = compute_phase2_targets();
    return targets;
  }

  static bool is_phase2_target(State key) {
    const std::vector<State>& targets = phase2_targets();
    return std::find(targets.begin(), targets.end(), key) != targets.end();
  }

  /**
   * Sequência de movimentos mais curta (entre os 8 geradores que
   * preservam a orientação) que leva a permutação de `start` a
   * coincidir com uma das 8 permutações resolvidas válidas. Busca no
   * espaço projetado de 40.320 chaves de permutação, não no estado
   * completo.
   */
  static Search solve_phase2(State start) {
    Search result;
    State startKey = perm_key(start);
    if (is_phase2_target(startKey)) return result;

    const std::array<Generator, 8>& gens = phase2_generators();
    struct Link { State parent; int genIndex; };
    std::unordered_map<State, Link> parentOf;
    std::deque<State> frontier{startKey};
    parentOf.emplace(startKey, Link{startKey, -1});

    while (!frontier.empty()) {
      State current = frontier.front(); frontier.pop_front();
      ++result.expanded;
      for (int i = 0; i < 8; ++i) {
        State next = apply_generator_to_perm_key(current, gens[i]);
        if (!parentOf.emplace(next, Link{current, i}).second) continue;
        if (is_phase2_target(next)) {
          std::vector<int> genPath;
          for (State k = next; !(k == startKey); ) {
            const Link& link = parentOf.at(k);
            genPath.push_back(link.genIndex);
            k = link.parent;
          }
          std::reverse(genPath.begin(), genPath.end());
          std::vector<std::string> path;
          for (int gi : genPath)
            for (int s = 0; s < gens[gi].stepCount; ++s) path.push_back(gens[gi].steps[s]->name);
          result.moves = std::move(path);
          return result;
        }
        frontier.push_back(next);
      }
    }
    return result;   // inalcançável na prática: a fase 2 cobre as 40320 chaves
  }
};
