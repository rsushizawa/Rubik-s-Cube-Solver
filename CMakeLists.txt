cmake_minimum_required(VERSION 3.15)
project(Cube LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Procura a instalação do raylib
find_package(Raylib 4.0 REQUIRED)

add_executable(Cube src/main.cpp)

# Vincula a biblioteca
target_link_libraries(Cube PRIVATE raylib)

# Configurações específicas para Windows (opcional)
if(WIN32)
    target_link_libraries(Cube PRIVATE gdi32 winmm)
    set_target_properties(Cube PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:\"mainCRTStartup\"")
endif()

# Configurações para macOS (frameworks)
if(APPLE)
    target_link_libraries(Cube PRIVATE "-framework IOKit")
    target_link_libraries(Cube PRIVATE "-framework Cocoa")
    target_link_libraries(Cube PRIVATE "-framework OpenGL")
endif()