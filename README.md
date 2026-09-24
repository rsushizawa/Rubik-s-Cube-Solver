# Rubik-s-Cube-Solver
Solving a 2x2x2 Rubik's Cube with BFS, DFS and A*

### On Fedora:
```bash
sudo dnf install cmake gcc-c++ raylib-devel
cmake -S . -B build
cmake --build build
./build/Cube
```

### On Windows:
```
cmake -S . -B build
cmake --build build --config Release
build\Release\Cube.exe
```
