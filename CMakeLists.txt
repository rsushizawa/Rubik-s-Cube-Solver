cmake_minimum_required(VERSION 3.15)
project(Cube LANGUAGES C CXX)

# Arquivos baixados pelo FetchContent ficam com a data da extração
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

# MSVC lê os fontes na página de código do sistema; os arquivos são UTF-8
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Usa o raylib instalado; se não houver (comum no Windows), baixa e compila
# o raylib 5.5 junto com o projeto
find_package(raylib 5.0 QUIET)
if(NOT raylib_FOUND)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    )
    FetchContent_MakeAvailable(raylib)
endif()

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

# Configurações específicas para Windows: sem janela de console
if(WIN32)
    target_link_libraries(Cube PRIVATE gdi32 winmm)
    if(MSVC)
        target_link_options(Cube PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
    else()
        target_link_options(Cube PRIVATE -mwindows)
    endif()
endif()

# Configurações para macOS (frameworks)
if(APPLE)
    target_link_libraries(Cube PRIVATE "-framework IOKit")
    target_link_libraries(Cube PRIVATE "-framework Cocoa")
    target_link_libraries(Cube PRIVATE "-framework OpenGL")
endif()

# Console de terminal (opcional: só se tools/ estiver presente)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tools/dev_console.cpp)
    add_executable(CubeCli
        tools/dev_console.cpp
    )
    target_include_directories(CubeCli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()
