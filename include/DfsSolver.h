#pragma once 

#include <exception>
#include <stack>
#include <unordered_map>
#include <algorithm>

#include "Solve.h"
#include "Transition.h"

// Depth first search Solver
class DfsSolver : public CubeSolver {
public:
    using CubeSolver::CubeSolver;

    // Parametros: start (estado inicial), goal(estado alvo)
    // Retorno: SearchResult
    SearchResult solve(uint64_t start, uint64_t goal = SOLVED_STATE) const override {
        SearchResult result;

        // Verifica se o start faz parte do Goal
        if(is_goal(start, goal)){
            result.found = true;
            return result;
        }

        // Cria a pilha para o DFS
        std::stack<uint64_t> frontier;
        frontier.push(start);

        // Cria o hashmap dos estados já visitado
        ParentLinks parentOf;
        parentOf.reserve(1u << 20);
        parentOf.emplace(start, Link{start, -1, 0});

        // Continua enquanto a pilha possuir algum elemento
        while(!frontier.empty()){
            // Desempilha o estado no topo
            uint64_t current = frontier.top();
            frontier.pop();

            // Conta a expansão
            ++result.expanded;
            
            int currentDepth = parentOf.at(current).depth;

            // Testa o limite de profundidade
            if(currentDepth >= 11){
                continue;
            }

            for(std::size_t i = 0; i < moves_.size(); ++i){
                // Aplicada o próximo estado de acordo com o movimento
                uint64_t nextState = apply_move(current, moves_[i]);
                int nextDepth = currentDepth + 1;
                
                // Verifica se o estado já foi visitado em uma profundidade menor
                auto it = parentOf.find(nextState);
            
                // Se o caminho registrado for menor ou igual ao que estamos verificando:
                if(it != parentOf.end() && it->second.depth <= nextDepth || nextDepth > 14){
                    continue; // Cortamos o ramo, por termos encontrado um caminho menor.
                } 

                // Grava o estado no hashmap
                parentOf[nextState] = Link{current, (int)i, nextDepth};

                // Verifica se esse novo estado é o final
                if(is_goal(nextState, goal)){
                    result.found = true; 
                    result.moves = trace_path(parentOf, start, nextState);
                    return result;
                }

                // Adiciona o próximo estado na pilha
                frontier.push(nextState);
            }
    }
        
    return result;

}

private:
    // Estrutura para armazenar estados, movimentos e profundidade (DFS)
    struct Link { uint64_t state; int moveIndex; int depth; };
    
    using ParentLinks = std::unordered_map<uint64_t, Link>;
    
    // Calcula o caminho da solução
    std::vector<std::string> trace_path(const ParentLinks& parentOf,
                                      uint64_t start, uint64_t goal) const {
    std::vector<std::string> path;
    for (uint64_t state = goal; state != start; ) {
      const Link& link = parentOf.at(state);
      path.push_back(moves_[link.moveIndex].name);
      state = link.state;
    }
    std::reverse(path.begin(), path.end());
    return path;
  }
};