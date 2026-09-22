#include "raylib.h"
#include "../include/CubeRenderer.hpp"
#include "../include/State.hpp"
#include "../include/Transition.hpp"
#include "../include/BfsSolver.hpp"
#include "../include/AStarSolver.hpp"

static const Move& move_by_name(const std::string& name) {
    for (const Move& m : ALL_MOVES) if (name == m.name) return m;
    return ALL_MOVES[0];  // unreachable: solver only ever emits names from ALL_MOVES
}

static const Vector3 SLOT_POSITIONS[8] = {
    {-0.52f,  0.52f,  0.52f}, { 0.52f,  0.52f,  0.52f},
    {-0.52f,  0.52f, -0.52f}, { 0.52f,  0.52f, -0.52f},
    {-0.52f, -0.52f,  0.52f}, { 0.52f, -0.52f,  0.52f},
    {-0.52f, -0.52f, -0.52f}, { 0.52f, -0.52f, -0.52f}
};

int main() {
    InitWindow(1280, 720, "Renderizador Cubo 2x2");

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 3.5f, 3.5f, 3.5f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Estado Inicial
    State cubeLogicState(0x1C1814100C080400ULL);
    CubeState cubeRender = InitCubeRenderState();
    UpdateCubeFromLogic(cubeLogicState, cubeRender);

    std::vector<Move> allMoves(std::begin(ALL_MOVES), std::end(ALL_MOVES));
    BfsSolver bfsSolver(allMoves);
    AStarSolver aStarSolver(allMoves);
    const int SOLVER_COUNT = 2;
    CubeSolver* solvers[SOLVER_COUNT] = { &bfsSolver, &aStarSolver };
    const char* solverNames[SOLVER_COUNT] = { "BFS", "A*" };

    enum class SolvePhase { None, Choosing, Playing };
    SolvePhase phase = SolvePhase::None;
    SearchResult candidates[SOLVER_COUNT];
    SearchResult solution;   // the one the user picked, mid-playback
    std::vector<std::string> solutionSteps;  // solution.moves, copied for playback
    std::size_t solveStep = 0;
    float stepTimer = 0.0f;
    const float STEP_INTERVAL = 0.35f;  // seconds between animated solve moves
    // Kept small on purpose: with per-expansion logging on, BFS's expanded
    // node count (and log line count) grows combinatorially past ~7 moves --
    // see the terminal output if you raise this.
    const int SCRAMBLE_LENGTH = 6;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // --- Face turns: U D L R F B  (hold SHIFT for the inverse) ---
        bool prime = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        const struct { int key; const Move* clockwise; const Move* counterClockwise; } faceKeyBindings[] = {
            { KEY_U, &MOVE_U, &MOVE_Up }, { KEY_D, &MOVE_D, &MOVE_Dp },
            { KEY_L, &MOVE_L, &MOVE_Lp }, { KEY_R, &MOVE_R, &MOVE_Rp },
            { KEY_F, &MOVE_F, &MOVE_Fp }, { KEY_B, &MOVE_B, &MOVE_Bp },
        };
        if (phase == SolvePhase::None) {
            for (const auto& binding : faceKeyBindings) {
                if (IsKeyPressed(binding.key)) {
                    const Move& applied = prime ? *binding.counterClockwise : *binding.clockwise;
                    TraceLog(LOG_INFO, "TURN: %s", applied.name);
                    cubeLogicState = transition(cubeLogicState, applied);
                    UpdateCubeFromLogic(cubeLogicState, cubeRender);
                }
            }
        }

        // --- Scramble: X applies SCRAMBLE_LENGTH random moves at once. ---
        if (IsKeyPressed(KEY_X) && phase == SolvePhase::None) {
            cubeLogicState = scramble(cubeLogicState, allMoves, SCRAMBLE_LENGTH);
            UpdateCubeFromLogic(cubeLogicState, cubeRender);
            TraceLog(LOG_INFO, "SCRAMBLE: %d moves", SCRAMBLE_LENGTH);
        }

        // --- Solve: SPACE runs each solver on the current state and shows
        // their results; press 1/2 to pick one and watch it play back one
        // move per STEP_INTERVAL, or ESC to cancel. ---
        if (IsKeyPressed(KEY_SPACE) && phase == SolvePhase::None) {
            for (int i = 0; i < SOLVER_COUNT; ++i) {
                TraceLog(LOG_INFO, "SOLVE[%s]: searching...", solverNames[i]);
                candidates[i] = solvers[i]->solve(cubeLogicState);
                if (candidates[i].found) {
                    TraceLog(LOG_INFO, "SOLVE[%s]: %s (%d moves, %llu nodes expanded)",
                             solverNames[i], candidates[i].notation().c_str(),
                             candidates[i].depth, (unsigned long long)candidates[i].expanded);
                } else {
                    TraceLog(LOG_WARNING, "SOLVE[%s]: no solution found within depth limit", solverNames[i]);
                }
            }
            phase = SolvePhase::Choosing;
        }
        if (phase == SolvePhase::Choosing) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                phase = SolvePhase::None;
            }
            for (int i = 0; i < SOLVER_COUNT; ++i) {
                if (IsKeyPressed(KEY_ONE + i) && candidates[i].found) {
                    TraceLog(LOG_INFO, "PICKED: %s", solverNames[i]);
                    solution = candidates[i];
                    solutionSteps = solution.moves;
                    solveStep = 0;
                    stepTimer = 0.0f;
                    phase = solutionSteps.empty() ? SolvePhase::None : SolvePhase::Playing;
                }
            }
        }
        if (phase == SolvePhase::Playing) {
            stepTimer += GetFrameTime();
            if (stepTimer >= STEP_INTERVAL) {
                stepTimer = 0.0f;
                const std::string& name = solutionSteps[solveStep];
                TraceLog(LOG_INFO, "MOVE %d/%d: %s", (int)solveStep + 1, (int)solutionSteps.size(), name.c_str());
                cubeLogicState = transition(cubeLogicState, move_by_name(name));
                UpdateCubeFromLogic(cubeLogicState, cubeRender);
                ++solveStep;
                if (solveStep >= solutionSteps.size()) phase = SolvePhase::None;
            }
        }

        BeginDrawing();
            ClearBackground((Color){ 30, 30, 30, 255 });

            BeginMode3D(camera);

                // Desenha o cubo
                RenderCube(cubeRender, 1.0f);
                DrawGrid(10, 1.0f);

            EndMode3D();

            for (int slot = 0; slot < 8; slot++) {
                    Vector2 screenPos = GetWorldToScreen(SLOT_POSITIONS[slot], camera);
                    DrawText(TextFormat("%d", slot), (int)screenPos.x - 6, (int)screenPos.y - 10, 22, BLACK);
            }
            DrawFPS(10, 10);
            DrawText("U D L R F B turn  |  SHIFT = inverse  |  X = scramble  |  SPACE = solve", 10, 35, 18, LIGHTGRAY);
            if (phase == SolvePhase::Choosing) {
                DrawText("pick a solution: 1/2  |  ESC = cancel", 10, 58, 18, LIGHTGRAY);
                for (int i = 0; i < SOLVER_COUNT; ++i) {
                    const char* status = !candidates[i].found ? "no solution"
                                        : TextFormat("%d moves: %s", candidates[i].depth,
                                                      candidates[i].notation().c_str());
                    DrawText(TextFormat("%d) %s -- %s", i + 1, solverNames[i], status),
                              10, 81 + i * 20, 18, LIGHTGRAY);
                }
            } else if (phase == SolvePhase::Playing) {
                DrawText(TextFormat("playing: %s (%d/%d)", solution.notation().c_str(),
                                     (int)solveStep, (int)solutionSteps.size()),
                          10, 58, 18, LIGHTGRAY);
            }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
