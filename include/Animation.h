#include "../include/Solve.h"
#include <raylib.h>

// Tracks current animations state
struct ActiveAnimation {                                                                                
        bool active = false;                                                                                
        Move move;                                                                                          
        Vector3 axis;                                                                                       
        float currentAngle = 0.0f;                                                                          
        float targetAngle = 0.0f;                                                                           
        float speed = 360.0f; // 360 deg/sec = 90 deg in 0.25 seconds                                       
};                                                                                                      

// Stores rotation axis and targetAngle of animation
struct AnimationInfo {                                                                              
    Vector3 axis;                                                                                       
    float targetAngle; // +90 or -90                                                                    
};     

// Retorna AnimationInfo com direção do ângulo de rotação e eixo de rotação
AnimationInfo getMoveAnimationInfo(const Move& m) {                                                 
    std::string name = m.name;                                                                          
    bool prime = (name.find('\'') != std::string::npos);                                                
    char face = name[0];                                                                                
                                                                                                            
    Vector3 axis = {0, 0, 0};                                                                           
    float sign = prime ? 1.0f : -1.0f;                                                                  
                                                                                                            
    switch (face) {                                                                                     
        case 'U': axis = {0, 1, 0}; sign *= 1.0f; break;                                                
        case 'D': axis = {0, 1, 0}; sign *= -1.0f; break;                                               
        case 'R': axis = {1, 0, 0}; sign *= 1.0f; break;                                                
        case 'L': axis = {1, 0, 0}; sign *= -1.0f; break;                                               
        case 'F': axis = {0, 0, 1}; sign *= 1.0f; break;                                                
        case 'B': axis = {0, 0, 1}; sign *= -1.0f; break;                                               
    }                                                                                                   
    return { axis, sign * 90.0f };                                                                      
}