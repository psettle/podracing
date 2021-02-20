#ifndef GAMESERVER_HPP
#define GAMESERVER_HPP

#include <cmath>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "IPlayer.hpp"
#include "Player.hpp"
#include "Pod.hpp"
#include "Vec2.hpp"

class GameController {
 public:
  void AddPlayer(IPlayer& player);

  // Return winning player (0 or 1)
  int RunGame();

 private:
  void InitMap();
  void InitPods();
  int Turn();
  int GetWinner() const;
  bool GetNextCheckpointCollision(double& dt, Pod*& pod);
  bool GetNextPlayerCollision(double& dt, Pod*& pod1, Pod*& pod2);
  static bool GetNextCollision(Vec2 const& p1, Vec2 const& v1, double r1, Vec2 const& p2,
                               Vec2 const& v2, double r2, double& dt);

  std::vector<Vec2> map_;
  std::vector<std::unique_ptr<Player>> players_;
  int frame_count = 0;
};

#endif