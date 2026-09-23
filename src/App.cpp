#include "App.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <random>

namespace {

namespace layout {
constexpr int SCREEN_W = 1280, SCREEN_H = 720;
constexpr int MARGIN = 10;
constexpr int TEXT = 18, TEXT_BIG = 20, TITLE = 22;

constexpr int HELP_Y = 35, HELP_LINE = 22;
constexpr int SOLVERS_TITLE_Y = 140, SOLVERS_Y = 168, SOLVER_ROW = 28,
              SOLVER_ROW_W = 230;
constexpr int SEED_Y = 290, SEED_BOX_W = 380;
constexpr int SCRAMBLE_LABEL_Y = 316, SCRAMBLE_Y = 340, SCRAMBLE_W = 380;
constexpr int STATUS_Y = 430;

constexpr int TABLE_X = 820, TABLE_Y = 20, TABLE_W = 460, TABLE_H = 190,
              TABLE_ROW = 26;
constexpr int COL_TIME = 150, COL_MOVES = 260, COL_VISITED = 330;

constexpr int SOLUTIONS_Y = 470, SOLUTION_FIRST_ROW = 28, SOLUTION_ROW = 52;
constexpr int SOLUTION_LABEL_X = 14, SOLUTION_CHIPS_X = 170,
              SOLUTION_CHIPS_W = 900, SOLUTION_STEP_X = 1090;

constexpr int CHIP_TEXT = 20, CHIP_PAD_X = 8, CHIP_H = 28, CHIP_GAP = 6;

int solution_row_y(int solver) {
  return SOLUTIONS_Y + SOLUTION_FIRST_ROW + solver * SOLUTION_ROW;
}
} // namespace layout

constexpr float CAMERA_DISTANCE = 6.06f;
constexpr float CAMERA_DRAG_SPEED = 0.01f;
constexpr float CAMERA_MAX_PITCH = 1.5f;

const Color BACKGROUND = {30, 30, 30, 255};
const Color PANEL = {22, 22, 26, 230};
const Color ROW_SELECTED = {60, 60, 66, 255};
const Color ROW_ACTIVE = {45, 45, 50, 255};

std::string fmt(const char *format, ...) {
  char buffer[512];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof buffer, format, args);
  va_end(args);
  return buffer;
}

void draw_text(const std::string &text, int x, int y, int size, Color color) {
  DrawText(text.c_str(), x, y, size, color);
}

std::vector<Rectangle> layout_chips(const std::vector<std::string> &moves,
                                    int x, int y, int maxWidth) {
  using namespace layout;
  std::vector<Rectangle> chips;
  int cx = x, cy = y;
  for (const std::string &name : moves) {
    int w = MeasureText(name.c_str(), CHIP_TEXT) + 2 * CHIP_PAD_X;
    if (cx + w > x + maxWidth) {
      cx = x;
      cy += CHIP_H + CHIP_GAP;
    }
    chips.push_back({(float)cx, (float)cy, (float)w, (float)CHIP_H});
    cx += w + CHIP_GAP;
  }
  return chips;
}

void draw_chips(const std::vector<std::string> &moves,
                const std::vector<Rectangle> &chips, int done, int highlight,
                Color accent) {
  using namespace layout;
  for (std::size_t i = 0; i < moves.size(); ++i) {
    const Rectangle &chip = chips[i];
    Color fill = {60, 60, 66, 255}, text = RAYWHITE;
    if ((int)i >= done) {
      fill = {38, 38, 42, 255};
      text = GRAY;
    }
    if ((int)i == highlight) {
      fill = accent;
      text = BLACK;
    }
    DrawRectangleRounded(chip, 0.3f, 6, fill);
    draw_text(moves[i], (int)chip.x + CHIP_PAD_X, (int)chip.y + 4, CHIP_TEXT,
              text);
  }
}

} // namespace

bool ResultSet::any() const {
  for (const auto &run : runs)
    if (run)
      return true;
  return false;
}

bool ResultSet::solved_by(int solver) const {
  return runs[solver] && runs[solver]->result.found;
}

State ResultSet::state_after(int solver, int k) const {
  const std::vector<std::string> &moves = runs[solver]->result.moves;
  return apply_moves(start, {moves.begin(), moves.begin() + k});
}

App::App()
    : moves_(std::begin(ALL_MOVES), std::end(ALL_MOVES)), bfs_(moves_),
      dfs_(moves_, 8),
      iddfs_(moves_), astar_(moves_), seed_(std::random_device{}()) {
  solvers_ = {
      {"BFS", SKYBLUE, &bfs_},
      {"DFS", ORANGE, &dfs_, &dfs_},
      {"IDDFS", VIOLET, &iddfs_},
      {"A*", LIME, &astar_},
  };
  results_.runs.resize(solvers_.size());
  astarTableMs_ = astar_.prepare();

  camera_.target = {0.0f, 0.0f, 0.0f};
  camera_.up = {0.0f, 1.0f, 0.0f};
  camera_.fovy = 45.0f;
  camera_.projection = CAMERA_PERSPECTIVE;
  place_camera();

  render_ = InitCubeRenderState();
  UpdateCubeFromLogic(cube_, render_);
}

void App::run() {
  InitWindow(layout::SCREEN_W, layout::SCREEN_H, "Renderizador Cubo 2x2");
  SetExitKey(KEY_NULL);
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    handle_input();
    update();
    draw();
  }
  CloseWindow();
}

void App::handle_input() {
  if (editingSeed_) {
    handle_seed_typing();
    return;
  }
  handle_solver_keys();
  handle_face_turns();
  handle_scramble_keys();
  handle_solution_keys();
  handle_camera_drag();
}

void App::handle_seed_typing() {
  for (int c = GetCharPressed(); c > 0; c = GetCharPressed())
    if (c >= '0' && c <= '9' && seedText_.size() < 10)
      seedText_ += (char)c;
  if (IsKeyPressed(KEY_BACKSPACE) && !seedText_.empty())
    seedText_.pop_back();
  if (IsKeyPressed(KEY_ESCAPE))
    editingSeed_ = false;
  if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
    if (seedText_.empty()) {
      seedError_ = "type a number first";
    } else if (std::stoull(seedText_) > 4294967295ULL) {
      seedError_ = "seed must be at most 4294967295";
    } else {
      seed_ = (uint32_t)std::stoull(seedText_);
      editingSeed_ = false;
      seedError_.clear();
      apply_seed();
    }
  }
}

void App::handle_solver_keys() {
  if (busy())
    return;
  for (int i = 0; i < (int)solvers_.size(); ++i)
    if (IsKeyPressed(KEY_ONE + i))
      selected_ = i;
  if (IsKeyPressed(KEY_TAB))
    selected_ = (selected_ + 1) % (int)solvers_.size();

  for (int i = 0; i < (int)solvers_.size(); ++i) {
    DfsSolver *limited = solvers_[i].limited;
    if (!limited)
      continue;
    int limit = limited->limit();
    if (IsKeyPressed(KEY_UP) && limit < 14)
      limited->set_limit(limit + 1);
    if (IsKeyPressed(KEY_DOWN) && limit > 1)
      limited->set_limit(limit - 1);
    if (limited->limit() != limit) {
      results_.runs[i].reset();
      if (cursor_.solver == i)
        cursor_ = {};
    }
  }
}

void App::handle_face_turns() {
  if (busy())
    return;
  const struct {
    int key;
    const Move *clockwise;
    const Move *counterClockwise;
  } bindings[] = {
      {KEY_U, &MOVE_U, &MOVE_Up}, {KEY_D, &MOVE_D, &MOVE_Dp},
      {KEY_L, &MOVE_L, &MOVE_Lp}, {KEY_R, &MOVE_R, &MOVE_Rp},
      {KEY_F, &MOVE_F, &MOVE_Fp}, {KEY_B, &MOVE_B, &MOVE_Bp},
  };
  bool prime = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
  for (const auto &binding : bindings)
    if (IsKeyPressed(binding.key))
      set_state(transition(
          cube_, prime ? *binding.counterClockwise : *binding.clockwise));
}

void App::handle_scramble_keys() {
  if (busy())
    return;
  if (IsKeyPressed(KEY_LEFT) && scrambleLength_ > 1)
    --scrambleLength_;
  if (IsKeyPressed(KEY_RIGHT) && scrambleLength_ < 14)
    ++scrambleLength_;
  if (IsKeyPressed(KEY_X)) {
    seed_ = std::random_device{}();
    apply_seed();
  }
  if (IsKeyPressed(KEY_Z))
    apply_seed();
  if (IsKeyPressed(KEY_S)) {
    editingSeed_ = true;
    seedText_.clear();
    seedError_.clear();
    while (GetCharPressed() > 0) {
    }
  }
  if (IsKeyPressed(KEY_C)) {
    apply_seed();
    if (!is_goal(cube_)) {
      std::vector<int> all;
      for (int i = 0; i < (int)solvers_.size(); ++i)
        all.push_back(i);
      start_search(all, false);
    }
  }
}

void App::handle_solution_keys() {
  if (IsKeyPressed(KEY_BACKSPACE))
    playing_ = false;
  if (busy())
    return;
  if (IsKeyPressed(KEY_SPACE))
    play_or_solve();
  if (IsKeyPressed(KEY_N))
    step_forward(selected_);
  if (IsKeyPressed(KEY_P))
    step_back(selected_);
  if (IsKeyPressed(KEY_HOME))
    jump_to(selected_, 0);
  if (IsKeyPressed(KEY_END) && results_.solved_by(selected_))
    jump_to(selected_, (int)results_.runs[selected_]->result.moves.size());
}

void App::handle_camera_drag() {
  if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    return;
  Vector2 delta = GetMouseDelta();
  cameraYaw_ -= delta.x * CAMERA_DRAG_SPEED;
  cameraPitch_ += delta.y * CAMERA_DRAG_SPEED;
  cameraPitch_ = std::clamp(cameraPitch_, -CAMERA_MAX_PITCH, CAMERA_MAX_PITCH);
  place_camera();
}

void App::place_camera() {
  camera_.position = {
      CAMERA_DISTANCE * std::cos(cameraPitch_) * std::sin(cameraYaw_),
      CAMERA_DISTANCE * std::sin(cameraPitch_),
      CAMERA_DISTANCE * std::cos(cameraPitch_) * std::cos(cameraYaw_),
  };
}

void App::update() {
  collect_search();

  if (playing_ && !animator_.active) {
    bool more = cursor_.solver >= 0 && on_solution(cursor_.solver) &&
                cursor_.step <
                    (int)results_.runs[cursor_.solver]->result.moves.size();
    if (more)
      step_forward(cursor_.solver);
    else
      playing_ = false;
  }

  if (animator_.update(render_)) {
    set_state(transition(cube_, animator_.currentMove));
    cursor_.step += animatedStep_;
    animatedStep_ = 0;
  }
}

bool App::busy() const {
  return job_.valid() || playing_ || animator_.active;
}

void App::set_state(State s) {
  cube_ = s;
  UpdateCubeFromLogic(cube_, render_);
}

void App::apply_seed() {
  set_state(scrambled_state(moves_, scrambleLength_, seed_, &scrambleMoves_));
}

void App::start_search(const std::vector<int> &which, bool playAfter) {
  jobSolvers_ = which;
  jobStart_ = cube_;
  jobPlaysAfter_ = playAfter;
  jobStartedAt_ = GetTime();

  std::vector<const CubeSolver *> chosen;
  for (int i : which)
    chosen.push_back(solvers_[i].solver);
  State start = cube_;
  std::vector<Move> moves = moves_;
  job_ = std::async(std::launch::async, [chosen, moves, start] {
    std::vector<SolverRun> out;
    for (const CubeSolver *solver : chosen) {
      SolverRun run;
      auto t0 = std::chrono::steady_clock::now();
      run.result = solver->solve(start);
      run.ms = std::chrono::duration<double, std::milli>(
                   std::chrono::steady_clock::now() - t0)
                   .count();
      run.verified = verify_solution(moves, start, run.result);
      out.push_back(run);
    }
    return out;
  });
}

void App::collect_search() {
  if (!job_.valid() ||
      job_.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
    return;

  std::vector<SolverRun> out = job_.get();
  if (!results_.any() || results_.start != jobStart_) {
    results_ = ResultSet{};
    results_.runs.resize(solvers_.size());
    results_.start = jobStart_;
    results_.fromSeed =
        scrambled_state(moves_, scrambleLength_, seed_) == jobStart_;
    results_.seed = seed_;
    results_.length = scrambleLength_;
    cursor_ = {};
  }
  for (std::size_t k = 0; k < out.size(); ++k) {
    int i = jobSolvers_[k];
    SolverRun &run = results_.runs[i].emplace(out[k]);
    if (solvers_[i].limited)
      run.limit = solvers_[i].limited->limit();
    TraceLog(LOG_INFO, "SOLVE[%s]: %s (%zu moves, %llu states visited, %.1f ms)",
             solvers_[i].name,
             run.result.found ? run.result.notation().c_str() : "no solution",
             run.result.moves.size(), (unsigned long long)run.result.expanded,
             run.ms);
  }
  if (jobPlaysAfter_ && results_.solved_by(selected_) && cube_ == jobStart_) {
    cursor_ = {selected_, 0};
    playing_ = true;
  }
}

void App::play_or_solve() {
  bool known = results_.solved_by(selected_) &&
               (on_solution(selected_) || cube_ == results_.start);
  if (known) {
    enter_solution(selected_);
    if (cursor_.step >= (int)results_.runs[selected_]->result.moves.size())
      jump_to(selected_, 0);
    playing_ = true;
  } else if (!is_goal(cube_)) {
    start_search({selected_}, true);
  }
}

bool App::on_solution(int solver) const {
  return cursor_.solver == solver && results_.solved_by(solver) &&
         results_.state_after(solver, cursor_.step) == cube_;
}

bool App::enter_solution(int solver) {
  if (on_solution(solver))
    return true;
  cursor_ = {solver, 0};
  if (cube_ == results_.start)
    return true;
  set_state(results_.start);
  return false;
}

void App::step_forward(int solver) {
  if (!results_.solved_by(solver) || !enter_solution(solver))
    return;
  const std::vector<std::string> &moves = results_.runs[solver]->result.moves;
  if (cursor_.step >= (int)moves.size())
    return;
  animator_.start(find_move(moves[cursor_.step]));
  animatedStep_ = +1;
}

void App::step_back(int solver) {
  if (!results_.solved_by(solver) || !enter_solution(solver) ||
      cursor_.step == 0)
    return;
  animator_.start(
      inverse_move(results_.runs[solver]->result.moves[cursor_.step - 1]));
  animatedStep_ = -1;
}

void App::jump_to(int solver, int step) {
  if (!results_.solved_by(solver))
    return;
  cursor_ = {solver, step};
  set_state(results_.state_after(solver, step));
}

void App::draw() {
  BeginDrawing();
  ClearBackground(BACKGROUND);

  BeginMode3D(camera_);
  RenderCube(render_, 1.0f);
  DrawGrid(10, 1.0f);
  EndMode3D();

  draw_slot_labels();
  draw_help();
  draw_solver_list();
  draw_seed_panel();
  draw_search_status();
  if (results_.any()) {
    draw_results_table();
    draw_solutions();
  }
  EndDrawing();
}

void App::draw_slot_labels() const {
  for (int slot = 0; slot < 8; slot++) {
    Vector2 p = GetWorldToScreen(SLOT_POSITIONS[slot], camera_);
    draw_text(std::to_string(slot), (int)p.x - 6, (int)p.y - 10, 22, BLACK);
  }
}

void App::draw_help() const {
  using namespace layout;
  const char *lines[] = {
      "U D L R F B turn  |  SHIFT = inverse",
      "X random seed  |  S type seed  |  Z redo seed  |  <- -> length",
      "1-4/TAB solver  |  UP/DOWN DFS limit  |  SPACE solve/play  |  C compare "
      "all",
      "N next  |  P previous  |  HOME/END  |  BACKSPACE stop  |  drag: rotate "
      "view",
  };
  DrawFPS(MARGIN, MARGIN);
  for (int i = 0; i < 4; ++i)
    draw_text(lines[i], MARGIN, HELP_Y + i * HELP_LINE, TEXT, LIGHTGRAY);
}

void App::draw_solver_list() const {
  using namespace layout;
  draw_text("Solver", MARGIN, SOLVERS_TITLE_Y, TITLE, RAYWHITE);
  for (int i = 0; i < (int)solvers_.size(); ++i) {
    int y = SOLVERS_Y + i * SOLVER_ROW;
    bool selected = i == selected_;
    if (selected)
      DrawRectangleRounded({6, (float)y - 3, SOLVER_ROW_W, 26}, 0.3f, 6,
                           ROW_SELECTED);
    DrawRectangle(14, y + 4, 12, 12, solvers_[i].color);
    std::string label = fmt("%d  %s", i + 1, solvers_[i].name);
    if (solvers_[i].limited)
      label += fmt(" (limit %d)", solvers_[i].limited->limit());
    draw_text(label, 34, y, TEXT_BIG, selected ? RAYWHITE : GRAY);
    if (solvers_[i].solver == &astar_)
      draw_text(fmt("tables built once: %.0f ms", astarTableMs_),
                SOLVER_ROW_W + 20, y + 2, TEXT, GRAY);
  }
}

void App::draw_seed_panel() const {
  using namespace layout;
  if (editingSeed_) {
    std::string box =
        "Seed: " + seedText_ + (std::fmod(GetTime(), 1.0) < 0.5 ? "_" : " ");
    DrawRectangleRounded({6, SEED_Y - 5, SEED_BOX_W, 30}, 0.2f, 6,
                         {70, 70, 30, 255});
    draw_text(box, 12, SEED_Y, TEXT_BIG, YELLOW);
    draw_text("ENTER apply  |  ESC cancel", MARGIN, SEED_Y + 32, TEXT, GRAY);
    if (!seedError_.empty())
      draw_text(seedError_, MARGIN, SEED_Y + 54, TEXT, RED);
    return;
  }
  draw_text(fmt("Seed %u  |  %d moves", seed_, scrambleLength_), MARGIN, SEED_Y,
            TEXT_BIG, RAYWHITE);
  if (!scrambleMoves_.empty()) {
    draw_text("Scramble:", MARGIN, SCRAMBLE_LABEL_Y, TEXT, GRAY);
    draw_chips(scrambleMoves_,
               layout_chips(scrambleMoves_, MARGIN, SCRAMBLE_Y, SCRAMBLE_W),
               (int)scrambleMoves_.size(), -1, GRAY);
  }
}

void App::draw_search_status() const {
  if (!job_.valid())
    return;
  std::string who;
  for (int i : jobSolvers_)
    who += std::string(who.empty() ? "" : ", ") + solvers_[i].name;
  draw_text(fmt("Solving (%s)... %.1f s", who.c_str(), GetTime() - jobStartedAt_),
            layout::MARGIN, layout::STATUS_Y, layout::TEXT_BIG, YELLOW);
}

std::string App::run_label(int solver, bool numbered) const {
  std::string label = numbered ? fmt("%d %s", solver + 1, solvers_[solver].name)
                               : std::string(solvers_[solver].name);
  const std::optional<SolverRun> &run = results_.runs[solver];
  if (run && run->limit >= 0)
    label += fmt(" (L=%d)", run->limit);
  return label;
}

void App::draw_results_table() const {
  using namespace layout;
  int x = TABLE_X, y = TABLE_Y;
  DrawRectangleRounded({(float)x - 10, (float)y - 10, TABLE_W, TABLE_H}, 0.05f,
                       6, PANEL);
  draw_text(results_.fromSeed ? fmt("Results  (seed %u, %d moves)",
                                    results_.seed, results_.length)
                              : std::string("Results  (custom state)"),
            x, y, TEXT_BIG, RAYWHITE);
  y += 32;
  draw_text("Solver", x, y, TEXT, GRAY);
  draw_text("Time", x + COL_TIME, y, TEXT, GRAY);
  draw_text("Moves", x + COL_MOVES, y, TEXT, GRAY);
  draw_text("Visited", x + COL_VISITED, y, TEXT, GRAY);

  for (int i = 0; i < (int)solvers_.size(); ++i) {
    y += TABLE_ROW;
    DrawRectangle(x, y + 4, 10, 10, solvers_[i].color);
    draw_text(run_label(i, false), x + 18, y, TEXT,
              i == selected_ ? RAYWHITE : LIGHTGRAY);

    const std::optional<SolverRun> &run = results_.runs[i];
    if (!run) {
      draw_text("-", x + COL_TIME, y, TEXT, GRAY);
      continue;
    }
    draw_text(run->ms < 1000 ? fmt("%.1f ms", run->ms)
                             : fmt("%.2f s", run->ms / 1000),
              x + COL_TIME, y, TEXT, LIGHTGRAY);
    if (!run->result.found) {
      draw_text("no solution", x + COL_MOVES, y, TEXT, RED);
      continue;
    }
    draw_text(fmt("%zu%s", run->result.moves.size(), run->verified ? "" : " !"),
              x + COL_MOVES, y, TEXT, run->verified ? LIGHTGRAY : RED);
    draw_text(fmt("%llu", (unsigned long long)run->result.expanded),
              x + COL_VISITED, y, TEXT, LIGHTGRAY);
  }
}

void App::draw_solutions() const {
  using namespace layout;
  draw_text("Solutions  (pick one with 1-4, step with N / P)",
            MARGIN, SOLUTIONS_Y, TEXT, GRAY);

  for (int i = 0; i < (int)solvers_.size(); ++i) {
    int y = solution_row_y(i);
    bool selected = i == selected_;
    if (selected)
      DrawRectangleRounded({4, (float)y - 6, SCREEN_W - 8, 44}, 0.15f, 6,
                           ROW_ACTIVE);
    draw_text(run_label(i, true), SOLUTION_LABEL_X, y + 4, TEXT_BIG,
              solvers_[i].color);

    const std::optional<SolverRun> &run = results_.runs[i];
    if (!run) {
      draw_text("not run  (SPACE runs it, C runs all)", SOLUTION_CHIPS_X,
                y + 4, TEXT, GRAY);
      continue;
    }
    if (!run->result.found) {
      draw_text(run->limit >= 0
                    ? fmt("no solution within depth limit %d  (raise it with "
                          "UP)",
                          run->limit)
                    : std::string("no solution"),
                SOLUTION_CHIPS_X, y + 4, TEXT, RED);
      continue;
    }

    const std::vector<std::string> &moves = run->result.moves;
    bool here = on_solution(i) || (animator_.active && cursor_.solver == i);
    int done = here ? cursor_.step : (int)moves.size();
    int highlight = -1;
    if (here)
      highlight = animator_.active && animatedStep_ > 0 ? cursor_.step
                                                        : cursor_.step - 1;
    draw_chips(moves,
               layout_chips(moves, SOLUTION_CHIPS_X, y, SOLUTION_CHIPS_W), done,
               highlight, solvers_[i].color);
    draw_text(here ? fmt("step %d/%zu", cursor_.step, moves.size())
                   : fmt("%zu moves", moves.size()),
              SOLUTION_STEP_X, y + 4, TEXT, selected ? RAYWHITE : GRAY);
  }
}
