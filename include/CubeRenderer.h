#pragma once

#include "raylib.h"
#include "State.h"
#include <array>

using namespace std;

struct CubieData {
    Vector3 position; //Posição do cubinho
    Vector3 rotationAxis; // Eixo de rotação
    float rotationAngle; // Angulo de rotação

    // Cores das 6 faces na ordem: 
    // [0]: Frente (+Z), [1]: Trás (-Z), [2]: Cima (+Y), 
    // [3]: Baixo (-Y),  [4]: Esquerda (-X), [5]: Direita (+X)
    array<Color,6> faceColors;    
};

// Conjunto completo do cubo contendo os 8 cubinhos.
struct CubeState {
    array<CubieData, 8> cubies;
};

// Conversão do estado lógico para o estado de renderização (CubeState)
CubeState UpdateCubeFromLogic(const State& logicState);

void DrawCube(const CubeState& cube, float cubieSize = 1.0f);
