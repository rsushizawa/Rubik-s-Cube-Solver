#include "raylib.h"
#include "../include/CubeRenderer.h"
#include "../include/State.h"
#include "../include/Transition.h"

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
    
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_FREE);

        // SIMULAÇÃO DE ANIMAÇÃO / TESTE DE INTERFACE:
        // Rotaciona as 4 peças superiores em torno do eixo Y para testar o renderizador
        /*
        static float animAngle = 0.0f;
        animAngle += 1.0f;
        for (int i = 0; i < 4; i++) {
            visualState.cubies[i].rotationAxis = (Vector3){ 0, 1, 0 };
            visualState.cubies[i].rotationAngle = animAngle;
        }
        
        */
        if (IsKeyPressed(KEY_U)) {
            cubeLogicState = transition(cubeLogicState, U_CYCLE, U_TWIST);
            UpdateCubeFromLogic(cubeLogicState, cubeRender); 
        }
        if (IsKeyPressed(KEY_R)) {
            cubeLogicState = transition(cubeLogicState, R_CYCLE, R_TWIST);
            UpdateCubeFromLogic(cubeLogicState, cubeRender); 
        }
        if (IsKeyPressed(KEY_F)) {
            cubeLogicState = transition(cubeLogicState, F_CYCLE, F_TWIST);
            UpdateCubeFromLogic(cubeLogicState, cubeRender); 
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
            DrawText("Rubik Cube Simulator", 10, 35, 18, LIGHTGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}