#include <cstdlib>

#include "App.hpp"

int main() {
  App app;
  app.run();

  if (app.searching())
    std::_Exit(0);
  return 0;
}
