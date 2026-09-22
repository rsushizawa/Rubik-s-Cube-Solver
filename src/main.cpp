#include "raylib.h"
#include "../include/CubeRenderer.h"
#include "../include/State.h"
#include "../include/Transition.h"
#include "../include/BfsSolver.h"
#include "../include/Solve.h"
#include "../include/Animation.h"
#include "../include/DfsSolver.h"

#include <deque>
#include <string>

static const Vector3 SLOT_POSITIONS[8] = {
    {-0.52f,  0.52f,  0.52f}, { 0.52f,  0.52f,  0.52f},
    {-0.52f,  0.52f, -0.52f}, { 0.52f,  0.52f, -0.52f},
    {-0.52f, -0.52f,  0.52f}, { 0.52f, -0.52f,  0.52f},
    {-0.52f, -0.52f, -0.52f}, { 0.52f, -0.52f, -0.52f}
};

// Function to apply Moves to a CubeLogicState (returns State)
State applyMoves(State state, const std::vector<std::string>& moveNames, const std::vector<Move>&       
  allMoves) {                                                                                               
        for (const std::string& name : moveNames) {                                                         
            for (const Move& m : allMoves) {                                                                
                if (m.name == name) {                                                                       
                    state = transition(state, m);                                                           
                    break;                                                                                  
                }                                                                                           
            }                                                                                               
        }                                                                                                   
        return state;                                                                                       
}

void executeMove(const Move& m, State& logicState, CubeState& renderState, CubeAnimator& animator) {    
    if (animator.enabled) {
        animator.start(m);                                                                              
    } else {
        logicState = transition(logicState, m);
        UpdateCubeFromLogic(logicState, renderState);
    }                                                                                                   
}             

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

    // ================ Movimentos Permitidos (Checar com 6 ou com todos) ===================
    //std::vector<Move> allMoves(std::begin(ALL_MOVES), std::end(ALL_MOVES));
    std::vector<Move> allMoves(std::begin(THREE_ADJACENT_MOVES), std::end(THREE_ADJACENT_MOVES));              

    // Create an instance for solvers
    BfsSolver Bfs(allMoves);
    DfsSolver Dfs(allMoves);

    // Armazena ultima solução
    SearchResult lastResult;

    // Estado atual da animação
    CubeAnimator animator;
    
    // Fila de movimentos para animar (se ativo)
    std::deque<Move> solveSteps;
    int currentStep = 0;

    bool autoPlay = false;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_ORBITAL);

        // Animation Update (verifica se animação está ativada)
        if(animator.active){
            // Realiza atualização da animação (Cuberender)
            if(animator.update(cubeRender)){
                // Quando finaliza a animação atualiza o estado lógico com o movimento realizado
                cubeLogicState = transition(cubeLogicState, animator.currentMove);
                UpdateCubeFromLogic(cubeLogicState, cubeRender);

                if (autoPlay) {
                    if (currentStep < (int)solveSteps.size()) {
                        executeMove(solveSteps[currentStep], cubeLogicState, cubeRender, animator);             
                        currentStep++;
                    } else {
                        autoPlay = false; // Fim da lista                                              
                    }
                }
            }
        }

        // Input do usuário apenas quando não há animação
        if(!animator.active){
            // --- Face turns: U D L R F B  (hold SHIFT for the inverse) ---
            bool prime = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
            const struct { int key; const Move* clockwise; const Move* counterClockwise; } faceKeyBindings[] = {
                { KEY_U, &MOVE_U, &MOVE_Up }, { KEY_D, &MOVE_D, &MOVE_Dp },
                { KEY_L, &MOVE_L, &MOVE_Lp }, { KEY_R, &MOVE_R, &MOVE_Rp },
                { KEY_F, &MOVE_F, &MOVE_Fp }, { KEY_B, &MOVE_B, &MOVE_Bp },
            };

            for (const auto& binding : faceKeyBindings) {
                if (IsKeyPressed(binding.key)) {
                    Move move = prime ? *binding.counterClockwise : *binding.clockwise;
                    executeMove(move, cubeLogicState, cubeRender, animator);
                    
                    // Se usuário executar movimento, limpa a fila de soluções
                    solveSteps.clear();
                    currentStep = 0;

                    //cubeLogicState = transition(cubeLogicState, prime ? *binding.counterClockwise : *binding.clockwise);
                    //UpdateCubeFromLogic(cubeLogicState, cubeRender);
                }
            }

            if (IsKeyPressed(KEY_A)) {                                                                  
                animator.enabled = !animator.enabled;                                                   
            }

            // Scrambles the Cube
            if(IsKeyPressed(KEY_S)){
                // Limpa a fila de solução
                solveSteps.clear();
                currentStep = 0;

                // Vetor com as rotações aleatórias (Seria interssante definir o número do scramble aqui)
                std::vector<std::string> scrambleMoves = scramble(allMoves, 4);
                cubeLogicState = applyMoves(cubeLogicState, scrambleMoves, allMoves);
                UpdateCubeFromLogic(cubeLogicState, cubeRender);
            }

            // Soluciona o cubo com BFS
            if (IsKeyPressed(KEY_N)) {                                                              
                SearchResult result = Bfs.solve(cubeLogicState.full_state);                          
                solveSteps.clear();
                currentStep = 0;                                                                      
                                                                                                            
                if (result.found) {
                    // Convert move names from result into Move structs=
                    for (const std::string& name : result.moves) {
                        for (const Move& m : allMoves) {
                            if (m.name == name) {
                                solveSteps.push_back(m);
                                break;
                            }                                                                           
                        }                                                                               
                    }
                }
            }

            if (IsKeyPressed(KEY_M)) {                                                              
                SearchResult result = Dfs.solve(cubeLogicState.full_state);                          
                solveSteps.clear();
                currentStep = 0;                                                                      
                                                                                                            
                if (result.found) {
                    // Convert move names from result into Move structs=
                    for (const std::string& name : result.moves) {
                        for (const Move& m : allMoves) {
                            if (m.name == name) {
                                solveSteps.push_back(m);
                                break;
                            }                                                                           
                        }                                                                               
                    }
                }
            }

            if (IsKeyPressed(KEY_RIGHT) && currentStep < (int)solveSteps.size()) {
                executeMove(solveSteps[currentStep], cubeLogicState, cubeRender, animator);
                currentStep++;
            }

            if (IsKeyPressed(KEY_LEFT) && currentStep > 0) {     
                // Returns to the last step                                                    
                currentStep--;
                // Get currentStep Move
                const Move& moveToUndo = solveSteps[currentStep];

                // Find moveToUndo in allMoves and grab its opposite (i ^ 1)                                        
                for (int i = 0; i < (int)allMoves.size(); ++i) {
                    if (allMoves[i].name == moveToUndo.name) {
                    Move undoMove = allMoves[i ^ 1]; // opposite move calculate from table

                    // Apply UndoMove
                    executeMove(undoMove, cubeLogicState, cubeRender, animator);
                    break;
                    }
                }                                                                                                   
            }

            if (IsKeyPressed(KEY_ENTER) && !solveSteps.empty()) {                                                   
                autoPlay = !autoPlay; // Toggle play / pause

                // If starting and idle, trigger the first move
                if (autoPlay && !animator.active && currentStep < (int)solveSteps.size()) {                         
                    executeMove(solveSteps[currentStep], cubeLogicState, cubeRender, animator);                     
                    currentStep++;
                }
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
            DrawText("U D L R F B turn  |  SHIFT = inverse", 10, 35, 18, LIGHTGRAY);
            DrawText("Press N to solve with BFS\nPress M to solve with DFS\nUse Arrow Keys to see the steps!", GetScreenWidth() - 500, 10, 20, WHITE);
            DrawText("S = Scramble", 10, 80, 30, WHITE);
            DrawText("Enter = Autoplay", 10, 140, 30, WHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
