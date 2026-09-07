#include "../include/CubeRenderer.h"
#include <cstdint>
#include <raylib.h>
#include <rlgl.h>

struct CubieColor { Color primary; Color sec; Color sec2; };

// Tabela de cores fixa de cada cubinho
static const CubieColor CubieColors[8] = {
    { WHITE,  GREEN,  RED },
    { WHITE,  GREEN,  ORANGE },
    { WHITE,  BLUE,   RED },
    { WHITE,  BLUE,   ORANGE },
    { YELLOW, GREEN,  RED },
    { YELLOW, GREEN,  ORANGE },
    { YELLOW, BLUE,   RED },
    { YELLOW, BLUE,   ORANGE }
};

// Tabela de posição dos cubinhos
static const Vector3 SLOT_POSITIONS[8] = {
    {-0.52f,  0.52f,  0.52f}, { 0.52f,  0.52f,  0.52f},
    {-0.52f,  0.52f, -0.52f}, { 0.52f,  0.52f, -0.52f},
    {-0.52f, -0.52f,  0.52f}, { 0.52f, -0.52f,  0.52f},
    {-0.52f, -0.52f, -0.52f}, { 0.52f, -0.52f, -0.52f}
};

// Inicializa o cubo com os dados setados de cada cubinho
CubeState InitCubeRenderState() {
    CubeState visualCube;
    
    for (int slot = 0; slot < 8; slot++) {
        visualCube.cubies[slot].position = SLOT_POSITIONS[slot];
        visualCube.cubies[slot].rotationAxis = (Vector3){ 0, 1, 0 };
        visualCube.cubies[slot].rotationAngle = 0.0f;
        visualCube.cubies[slot].faceColors.fill((Color){ 20, 20, 20, 255 });
    }
    
    return visualCube;
}

// Atualiza os cubies do Cubo (a partir do estado lógico)
void UpdateCubeFromLogic(const State &logicState, CubeState &cube){
    for(int slot = 0; slot < 8; slot++){
        uint8_t raw = logicState.slots[slot]; // Estado lógico de um cubinho
        uint8_t id = raw >> 2; // ID do cubinho (3bits mais significativos)
        uint8_t ori = raw & 0x03; // Orientação do cubinho (2 bits menos significativos)

        // Cores do cubinho específico a partir do ID
        CubieColor cc = CubieColors[id];

        // Mapeia os índices das 3 faces externas na ordem do array:
        // faces[0] = Y, faces[1] = Z, faces[2] = X
        int faces[3];
        faces[0] = (slot < 4) ? 2 : 3;                                          // Y: 2 (+Y), 3 (-Y)
        faces[1] = (slot == 0 || slot == 1 || slot == 4 || slot == 5) ? 0 : 1; // Z: 0 (+Z), 1 (-Z)
        faces[2] = (slot % 2 == 1) ? 5 : 4;                                     // X: 5 (+X), 4 (-X)

        // Limpeza das faces anteriores
        cube.cubies[slot].faceColors.fill((Color){ 20, 20, 20, 255 });
        
        // Calcula a quiralidade 3D (Handedness) do slot a partir do sinal (X * Y * Z)
        auto getHandedness = [](int s) {
            bool posX = (s % 2 == 1);
            bool posY = (s < 4);
            bool posZ = (s == 0 || s == 1 || s == 4 || s == 5);
            return (posX ^ posY ^ posZ);
        };

        bool sameHandedness = (getHandedness(id) == getHandedness(slot));

        // Atribui as faces utilizando permutação cíclica
        int primaryIdx = ori;
        int secIdx     = sameHandedness ? (ori + 1) % 3 : (ori + 2) % 3;
        int sec2Idx    = sameHandedness ? (ori + 2) % 3 : (ori + 1) % 3;

        cube.cubies[slot].faceColors[faces[primaryIdx]] = cc.primary;
        cube.cubies[slot].faceColors[faces[secIdx]]     = cc.sec;
        cube.cubies[slot].faceColors[faces[sec2Idx]]    = cc.sec2;
    }
}

// Desenha um cubinho (CubieData)
static void DrawCubie(const CubieData &cubie, float size){
    rlPushMatrix();
        // Aplica rotação e translação no espaço 3D
        rlRotatef(cubie.rotationAngle, cubie.rotationAxis.x, cubie.rotationAxis.y, cubie.rotationAxis.z);
        rlTranslatef(cubie.position.x, cubie.position.y, cubie.position.z);

        // 1. Desenha o bloco principal de plástico
        DrawCube((Vector3){0,0,0}, size, size, size, BLACK);

        // 2. Configuração dos Stickers
        float half = size / 2.0f;
        float stickerSize = size * 0.88f;
        float thickness = 0.01f;
        float offset = half + (thickness / 2.0f) + 0.001f;

        // Frente (+Z)
        DrawCube((Vector3){ 0, 0, offset }, stickerSize, stickerSize, thickness, cubie.faceColors[0]);
        // Trás (-Z)
        DrawCube((Vector3){ 0, 0, -offset }, stickerSize, stickerSize, thickness, cubie.faceColors[1]);
        // Cima (+Y)
        DrawCube((Vector3){ 0, offset, 0 }, stickerSize, thickness, stickerSize, cubie.faceColors[2]);
        // Baixo (-Y)
        DrawCube((Vector3){ 0, -offset, 0 }, stickerSize, thickness, stickerSize, cubie.faceColors[3]);
        // Esquerda (-X)
        DrawCube((Vector3){ -offset, 0, 0 }, thickness, stickerSize, stickerSize, cubie.faceColors[4]);
        // Direita (+X)
        DrawCube((Vector3){ offset, 0, 0 }, thickness, stickerSize, stickerSize, cubie.faceColors[5]);
    rlPopMatrix();
}

// Desenha o cubo inteiro (todos os Cubies)
void RenderCube(CubeState cube, float cubie_size){
    for(const auto &cubie : cube.cubies){
        DrawCubie(cubie, cubie_size);
    }
}