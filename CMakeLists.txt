cmake_minimum_required(VERSION 3.15)
project(Cube LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Procura a instalação do raylib
find_package(Raylib 4.0 REQUIRED)
# As buscas rodam numa thread separada (std::async) para a janela não travar
find_package(Threads REQUIRED)

add_executable(Cube 
    src/main.cpp
    src/App.cpp
    src/CubeRenderer.cpp
)

target_include_directories(Cube PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Vincula a biblioteca
target_link_libraries(Cube PRIVATE raylib Threads::Threads)

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

add_executable(CubeCli
    tools/dev_console.cpp
)
target_include_directories(CubeCli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
