#include <cstdlib>
#include <iostream>
#include "GameServer.hpp"
#include "dumbcontroller.hpp"

int main() {
  DumbController controller1;
  DumbController controller2;

  std::srand(1);

  GameController server;
  server.AddPlayer(controller1);
  server.AddPlayer(controller2);

  std::cout << server.RunGame() << std::endl;
  return 0;
}