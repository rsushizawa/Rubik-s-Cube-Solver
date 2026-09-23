#pragma once

#include <cstdint>
#include <future>
#include <optional>
#include <string>
#include <vector>

#include "../include/AStarSolver.hpp"
#include "../include/Animation.hpp"
#include "../include/BfsSolver.hpp"
#include "../include/CubeRenderer.hpp"
#include "../include/DfsSolver.hpp"
#include "../include/IddfsSolver.hpp"
#include "raylib.h"

struct SolverEntry {
  const char *name;
  Color color;
  const CubeSolver *solver;
  DfsSolver *limited = nullptr;
};

struct SolverRun {
  SearchResult result;
  double ms = 0.0;
  bool verified = false;
  int limit = -1;
};

/**
 * Os resultados de todos os solucionadores para o MESMO estado inicial.
 * Cada solução fica guardada, e qualquer uma pode ser percorrida passo a
 * passo a partir de `start`.
 *
 * Atributos
 * ----------
 * start: State
 *     Estado que todos os solucionadores resolveram.
 * fromSeed: bool
 *     Se `start` veio do embaralhamento de `seed` com `length` movimentos.
 * seed: uint32_t
 *     Seed do embaralhamento (só vale se fromSeed).
 * length: int
 *     Tamanho do embaralhamento (só vale se fromSeed).
 * runs: std::vector<std::optional<SolverRun>>
 *     Um por solucionador, na ordem de App::solvers_; vazio se ele ainda
 *     não rodou neste estado.
 */
struct ResultSet {
  State start;
  bool fromSeed = false;
  uint32_t seed = 0;
  int length = 0;
  std::vector<std::optional<SolverRun>> runs;

  bool any() const;
  bool solved_by(int solver) const;
  State state_after(int solver, int k) const;
};

struct SolutionCursor {
  int solver = -1;
  int step = 0;
};

/**
 * A interface gráfica: o cubo em 3D, os solucionadores, a seed e as
 * soluções encontradas.
 *
 * Cada quadro é handle_input() -> update() -> draw(). A entrada muda o
 * estado da aplicação, a atualização avança buscas e animações, e o
 * desenho só lê o estado (os métodos de desenho são const). As buscas
 * rodam numa thread separada (start_search / collect_search), para a
 * janela não travar enquanto um solucionador lento trabalha.
 */
class App {
public:
  App();

  /**
   * Abre a janela e roda o laço de quadros até ela ser fechada.
   */
  void run();

  /**
   * Retorna
   * -------
   * bool
   *     Verdadeiro se uma busca ainda está rodando em segundo plano.
   */
  bool searching() const { return job_.valid(); }

private:
  /**
   * Lê o teclado e o mouse e aplica o que foi pedido: girar faces,
   * embaralhar, escolher o solucionador, resolver, percorrer a solução.
   * Enquanto a seed está sendo digitada, só a digitação é tratada.
   */
  void handle_input();

  /**
   * Avança o que não depende de entrada: recolhe o resultado de uma busca
   * que terminou, toca a solução sozinho (SPACE) e aplica ao estado
   * lógico o movimento que acabou de ser animado.
   */
  void update();

  /**
   * Desenha o quadro: o cubo em 3D, a ajuda, a lista de solucionadores, a
   * seed, o status da busca, a tabela de resultados e as soluções.
   */
  void draw();

  void handle_seed_typing();
  void handle_solver_keys();
  void handle_face_turns();
  void handle_scramble_keys();
  void handle_solution_keys();
  void handle_camera_drag();
  void place_camera();

  bool busy() const;
  void set_state(State s);
  void apply_seed();

  /**
   * Começa, numa thread separada, a busca de cada solucionador em `which`
   * a partir do estado atual do cubo, medindo o tempo e conferindo cada
   * solução com verify_solution().
   *
   * Parâmetros
   * ----------
   * which: const std::vector<int>&
   *     Índices, em solvers_, dos solucionadores a rodar.
   * playAfter: bool
   *     Se, ao terminar, a solução do solucionador escolhido deve começar
   *     a tocar (SPACE) ou só ser guardada (C).
   */
  void start_search(const std::vector<int> &which, bool playAfter);

  /**
   * Se a busca em segundo plano terminou, guarda os resultados em
   * results_ (começando um ResultSet novo se o estado inicial mudou) e,
   * se ela veio de um SPACE, começa a tocar a solução.
   */
  void collect_search();

  void play_or_solve();
  bool on_solution(int solver) const;
  bool enter_solution(int solver);
  void step_forward(int solver);
  void step_back(int solver);
  void jump_to(int solver, int step);

  void draw_slot_labels() const;
  void draw_help() const;
  void draw_solver_list() const;
  void draw_seed_panel() const;
  void draw_search_status() const;
  void draw_results_table() const;
  void draw_solutions() const;
  std::string run_label(int solver, bool numbered) const;

  Camera3D camera_{};
  float cameraYaw_ = 0.785f;
  float cameraPitch_ = 0.615f;
  State cube_ = SOLVED_STATE;
  CubeState render_;
  CubeAnimator animator_;
  int animatedStep_ = 0;

  std::vector<Move> moves_;
  BfsSolver bfs_;
  DfsSolver dfs_;
  IddfsSolver iddfs_;
  AStarSolver astar_;
  double astarTableMs_ = 0.0;
  std::vector<SolverEntry> solvers_;
  int selected_ = 0;

  uint32_t seed_;
  int scrambleLength_ = 14;
  std::vector<std::string> scrambleMoves_;

  bool editingSeed_ = false;
  std::string seedText_;
  std::string seedError_;

  ResultSet results_;
  SolutionCursor cursor_;
  bool playing_ = false;

  std::future<std::vector<SolverRun>> job_;
  std::vector<int> jobSolvers_;
  State jobStart_;
  bool jobPlaysAfter_ = false;
  double jobStartedAt_ = 0.0;
};
