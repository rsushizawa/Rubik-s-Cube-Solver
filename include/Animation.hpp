#pragma once

#include "raylib.h"
#include "../include/CubeRenderer.h"
#include "../include/State.h"
#include "../include/Solve.h"

// Self-contained animator                                         
class CubeAnimator {                                                                          
    public:
    bool enabled = true;                                                                                
    bool active = false;                                                                                
    Move currentMove;                                                                                   
    Vector3 axis = {0, 0, 0};                                                                           
    float currentAngle = 0.0f;                                                                          
    float targetAngle = 0.0f;                                                                           
    float speed = 360.0f; // degrees / second                                                           

    
    void start(const Move& m) {                                                                         
        if (!this->enabled) return; // If disabled, don't start animation                                     
        active = true;                                                                                  
        this->currentMove = m;                                                                                
        this->currentAngle = 0.0f;                                                                            
                                                                                                            
        // Determine axis & angle                                    
        bool prime = (std::string(m.name).find('\'') != std::string::npos);                             
        float sign = prime ? 1.0f : -1.0f;                                                              
        char face = m.name[0];                                                                          
                                                                                                            
        switch (face) {                                                                                     
            case 'U': axis = {0, 1, 0}; this->targetAngle =  sign * 90.0f; break;                                                
            case 'D': axis = {0, 1, 0}; this->targetAngle = -sign * 90.0f; break;                                               
            case 'R': axis = {1, 0, 0}; this->targetAngle =  sign * 90.0f; break;                                                
            case 'L': axis = {1, 0, 0}; this->targetAngle = -sign * 90.0f; break;                                               
            case 'F': axis = {0, 0, 1}; this->targetAngle =  sign * 90.0f; break;                                                
            case 'B': axis = {0, 0, 1}; this->targetAngle = -sign * 90.0f; break;                                               
        }                                             
    }                                                                                                   
                                                                                                            
    // Returns true when the animation reaches 90 degrees (finished)                                    
    bool update(CubeState& cubeRender) {                                                                
        if (!this->active) return false;                                                                      
                                                                                                            
        float step = this->speed * GetFrameTime();                                                            
        this->currentAngle += (this->targetAngle > 0 ? 1.0f : -1.0f) * step;                                        
                                                                                                            
        if (std::abs(this->currentAngle) >= std::abs(this->targetAngle)) {                                          
            // Done! Reset cubie rotation angles to 0                                                   
            for (int slot : this->currentMove.cycle) {                                                        
                cubeRender.cubies[slot].rotationAngle = 0.0f;                                           
            }                                                                                           
            this->active = false;                                                                             
            return true; // Finished this frame                                                         
        }                                                                                               
                                                                                                            
        // Apply intermediate angles                                                                    
        for (int slot : this->currentMove.cycle) {                                                            
            cubeRender.cubies[slot].rotationAxis = this->axis;                                                
            cubeRender.cubies[slot].rotationAngle = this->currentAngle;                                       
        }                                                                                               
        return false;                                                                                   
    }                                                                                                   
};           
