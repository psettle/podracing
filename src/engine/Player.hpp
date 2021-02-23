#ifndef PLAYER_H
#define PLAYER_H

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "IPlayer.hpp"
#include "Pod.hpp"
#include "Vec2.hpp"

class Player {
 public:
  Player(IPlayer& controller);
  void Setup(std::string const& data);
  void InitPods(Vec2 const& origin, Vec2 const& direction, double seperation, Vec2 const& target);
  void SetInitialTurnConditions(std::string const& input_data, bool first_frame);
  void GetGameInput(std::ostringstream& game_input);
  void EndTurn();
  void AdvancePods(double dt);

  std::vector<std::unique_ptr<Pod>> const& pods() const { return pods_; }
  bool has_won() const { return has_won_; }
  double win_time() const { return win_time_; }
  bool has_lost() const { return has_lost_; }

 private:
  std::vector<PodControl> CollectBotOutput(std::string const& input_data);

  IPlayer& controller_;
  std::vector<std::unique_ptr<Pod>> pods_;
  std::ostringstream output_;
  std::istringstream input_;
  int timeout_;
  int boosts_available_;
  bool has_won_ = false;
  double win_time_ = 1.0;
  bool has_lost_ = false;
};

#endif