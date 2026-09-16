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
        UpdateCamera(&camera, CAMERA_ORBITAL);

        // --- Face turns: U D L R F B  (hold SHIFT for the inverse) ---
        bool prime = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        const struct { int key; const Move* clockwise; const Move* counterClockwise; } faceKeyBindings[] = {
            { KEY_U, &MOVE_U, &MOVE_Up }, { KEY_D, &MOVE_D, &MOVE_Dp },
            { KEY_L, &MOVE_L, &MOVE_Lp }, { KEY_R, &MOVE_R, &MOVE_Rp },
            { KEY_F, &MOVE_F, &MOVE_Fp }, { KEY_B, &MOVE_B, &MOVE_Bp },
        };
        for (const auto& binding : faceKeyBindings) {
            if (IsKeyPressed(binding.key)) {
                cubeLogicState = transition(cubeLogicState, prime ? *binding.counterClockwise : *binding.clockwise);
                UpdateCubeFromLogic(cubeLogicState, cubeRender);
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

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
