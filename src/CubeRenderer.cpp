#include "../include/CubeRenderer.hpp"
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
const Vector3 SLOT_POSITIONS[8] = {
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
        visualCube.cubies[slot].rotationAxis = Vector3{ 0, 1, 0 };
        visualCube.cubies[slot].rotationAngle = 0.0f;
        visualCube.cubies[slot].faceColors.fill(Color{ 20, 20, 20, 255 });
    }
    
    return visualCube;
}

static constexpr uint8_t SLOT_ORI_FACES[8][3][3] =
{
    // slot 0 (-X, +Y, +Z)
    {
        {2, 0, 4}, // ori 0
        {4, 2, 0}, // ori 1
        {0, 4, 2}  // ori 2
    },

    // slot 1 (+X, +Y, +Z)
    {
        {2, 0, 5}, // ori 0
        {0, 5, 2}, // ori 1
        {5, 2, 0}  // ori 2
    },

    // slot 2 (-X, +Y, -Z)
    {
        {2, 1, 4}, // ori 0
        {1, 4, 2}, // ori 1
        {4, 2, 1}  // ori 2
    },

    // slot 3 (+X, +Y, -Z)
    {
        {2, 1, 5}, // ori 0
        {5, 2, 1}, // ori 1
        {1, 5, 2}  // ori 2
    },

    // slot 4 (-X, -Y, +Z)
    {
        {3, 0, 4}, // ori 0
        {0, 4, 3}, // ori 1
        {4, 3, 0}  // ori 2
    },

    // slot 5 (+X, -Y, +Z)
    {
        {3, 0, 5}, // ori 0
        {5, 3, 0}, // ori 1
        {0, 5, 3}  // ori 2
    },

    // slot 6 (-X, -Y, -Z)
    {
        {3, 1, 4}, // ori 0
        {4, 3, 1}, // ori 1
        {1, 4, 3}  // ori 2
    },

    // slot 7 (+X, -Y, -Z)
    {
        {3, 1, 5}, // ori 0
        {1, 5, 3}, // ori 1
        {5, 3, 1}  // ori 2
    }
};

// Atualiza os cubies do Cubo (a partir do estado lógico)
void UpdateCubeFromLogic(const State &logicState, CubeState &cube){
    for(int slot = 0; slot < 8; slot++){
        uint8_t raw = logicState.slots[slot]; // Estado lógico de um cubinho
        uint8_t id = raw >> 2; // ID do cubinho (3bits mais significativos)
        uint8_t ori = raw & 0x03; // Orientação do cubinho (2 bits menos significativos)

        // Cores do cubinho específico a partir do ID
        CubieColor cc = CubieColors[id];

        // Limpa as 6 faces com a cor base (plástico interno)
        cube.cubies[slot].faceColors.fill(Color{ 20, 20, 20, 255 });

        /*
         * LUT:
         *
         * map[0] = face da PRIMARY
         * map[1] = face da SECONDARY
         * map[2] = face da TERTIARY
         */
        const auto& map = SLOT_ORI_FACES[slot][ori];

        int primaryFace   = map[0];
        int secondaryFace = map[1];
        int tertiaryFace  = map[2];

        /*
         * A quiralidade do cubinho é uma propriedade do ID.
         * A quiralidade da posição é uma propriedade do SLOT.
         *
         * Quando são diferentes, secondary e tertiary precisam
         * trocar de lugar.
         */
        auto handedness = [](int s) -> bool
        {
            bool posX = (s % 2 == 1);
            bool posY = (s < 4);
            bool posZ = (s == 0 || s == 1 || s == 4 || s == 5);

            return posX ^ posY ^ posZ;
        };

        bool sameHandedness =
            handedness(id) == handedness(slot);

        if (!sameHandedness)
        {
            std::swap(secondaryFace, tertiaryFace);
        }

        cube.cubies[slot].faceColors[primaryFace]   = cc.primary;
        cube.cubies[slot].faceColors[secondaryFace] = cc.sec;
        cube.cubies[slot].faceColors[tertiaryFace]  = cc.sec2;
    }
}

// Desenha um cubinho (CubieData)
static void DrawCubie(const CubieData &cubie, float size){
    rlPushMatrix();
        // Aplica rotação e translação no espaço 3D
        rlRotatef(cubie.rotationAngle, cubie.rotationAxis.x, cubie.rotationAxis.y, cubie.rotationAxis.z);
        rlTranslatef(cubie.position.x, cubie.position.y, cubie.position.z);

        // 1. Desenha o bloco principal de plástico
        DrawCube(Vector3{0,0,0}, size, size, size, BLACK);

        // 2. Configuração dos Stickers
        float half = size / 2.0f;
        float stickerSize = size * 0.88f;
        float thickness = 0.01f;
        float offset = half + (thickness / 2.0f) + 0.001f;

        // Frente (+Z)
        DrawCube(Vector3{ 0, 0, offset }, stickerSize, stickerSize, thickness, cubie.faceColors[0]);
        // Trás (-Z)
        DrawCube(Vector3{ 0, 0, -offset }, stickerSize, stickerSize, thickness, cubie.faceColors[1]);
        // Cima (+Y)
        DrawCube(Vector3{ 0, offset, 0 }, stickerSize, thickness, stickerSize, cubie.faceColors[2]);
        // Baixo (-Y)
        DrawCube(Vector3{ 0, -offset, 0 }, stickerSize, thickness, stickerSize, cubie.faceColors[3]);
        // Esquerda (-X)
        DrawCube(Vector3{ -offset, 0, 0 }, thickness, stickerSize, stickerSize, cubie.faceColors[4]);
        // Direita (+X)
        DrawCube(Vector3{ offset, 0, 0 }, thickness, stickerSize, stickerSize, cubie.faceColors[5]);
    rlPopMatrix();
}

// Desenha o cubo inteiro (todos os Cubies)
void RenderCube(CubeState cube, float cubie_size){
    for(const auto &cubie : cube.cubies){
        DrawCubie(cubie, cubie_size);
    }
}
