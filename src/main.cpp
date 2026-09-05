#include <raylib.h>

int main(){
    const int screenWidth = 1200;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "Visualizador de Array 3D");

    // Configuração da Câmera 3D
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 8.0f, 6.0f, 8.0f }; // Posição da câmera no espaço
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };   // Ponto para onde ela olha
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };       // Define o eixo Y como "para cima"
    camera.fovy = 45.0f;                             // Campo de visão (Field of view) em graus
    camera.projection = CAMERA_PERSPECTIVE;          // Perspectiva com profundidade

    // 1 = desenha um cubo, 0 = espaço vazio
    const int tamanho = 2;

    SetTargetFPS(60);

    // 4. Laço principal
    while (!WindowShouldClose()) {
        
        // Atualiza a câmera (CAMERA_ORBITAL permite girar com o mouse/setas)
        UpdateCamera(&camera, CAMERA_ORBITAL);

        // 5. Início do desenho
        BeginDrawing();
            ClearBackground(RAYWHITE);

            // Inicia o contexto 3D com as configurações da nossa câmera
            BeginMode3D(camera);

            // Varredura do array e renderização
            // Face 1
            for (int y = 0; y < tamanho; y++) {
                for (int z = 0; z < tamanho; z++) {
                    Vector3 posicao = { 0, (float)y , (float)z };
                    
                    // Desenha o cubo sólido
                    DrawCube(posicao, 0.1f, 1.0f, 1.0f, RED);

                    // Desenha as arestas para visualização clara (Wireframe)
                    DrawCubeWires(posicao, 0.1f, 1.0f, 1.0f, MAROON);
                    
                }
                    
            }

            // Face 2
            for (int y = 0; y < tamanho; y++) {
                for (int z = 0; z < tamanho; z++) {
                    Vector3 posicao = { 2, (float)y, (float)z};
                    
                    // Desenha o cubo sólido
                    DrawCube(posicao, 0.1f, 1.0f, 1.0f, BLUE);

                    // Desenha as arestas para visualização clara (Wireframe)
                    DrawCubeWires(posicao, 0.1f, 1.0f, 1.0f, MAROON);
                    
                    }
                    
            }
            
            // Face Bottom
            for (int x = 0; x < tamanho; x++) {
                for (int z = 0; z < tamanho; z++) {
                    Vector3 posicao = { (float)x + 0.5f, -0.5, (float)z};
                    
                    // Desenha o cubo sólido
                    DrawCube(posicao, 1.0f, 0.1f, 1.0f, YELLOW);

                    // Desenha as arestas para visualização clara (Wireframe)
                    DrawCubeWires(posicao, 1.0f, 0.1f, 1.0f, MAROON);
                    
                }
                    
            }

            // Face Top
            for (int x = 0; x < tamanho; x++) {
                for (int z = 0; z < tamanho; z++) {     
                    Vector3 posicao = { (float)x +0.5f, 1.5, (float)z };
                    
                    // Desenha o cubo sólido
                    DrawCube(posicao, 1.0f, 0.1f, 1.0f, WHITE);

                    // Desenha as arestas para visualização clara (Wireframe)
                    DrawCubeWires(posicao, 1.0f, 0.1f, 1.0f, MAROON);
                    
                }
                    
            }

            // Face Direita
            for (int x = 0; x < tamanho; x++) {
                for (int y = 0; y < tamanho; y++) {
                    Vector3 posicao = { (float)x +0.5f, (float)y, 1.5};
                    
                    // Desenha o cubo sólido
                    DrawCube(posicao, 1.0f, 1.0f, 0.1f, GREEN);

                    // Desenha as arestas para visualização clara (Wireframe)
                    DrawCubeWires(posicao, 1.0f, 1.0f, 0.1f, MAROON);
                    
                }
            }

            // Face Esquerda
            for (int x = 0; x < tamanho; x++) {
                for (int y = 0; y < tamanho; y++) {
                    Vector3 posicao = { (float)x +0.5f, (float)y, -0.5};
                    
                    // Desenha o cubo sólido
                    DrawCube(posicao, 1.0f, 1.0f, 0.1f, PINK);

                    // Desenha as arestas para visualização clara (Wireframe)
                    DrawCubeWires(posicao, 1.0f, 1.0f, 0.1f, MAROON);
                    
                }
            }

            // Desenha uma grade no chão para referência espacial
            //DrawGrid(10, 1.0f);

            // Encerra o contexto 3D
            EndMode3D();

            DrawFPS(10, 10);
            
        EndDrawing();
    }

    // 7. Limpeza da memória e fechamento
    CloseWindow();
    return 0;
}