#include <cstdlib>
#include <iostream>
#include "GameServer.hpp"
#include "dumbcontroller.hpp"

int main() {
  DumbController controller1;
  DumbController controller2;

  std::srand(2);

  GameController server(controller1, controller2);

  std::cout << server.RunGame() << std::endl;
  return 0;
}