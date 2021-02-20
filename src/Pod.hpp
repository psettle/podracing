#ifndef POD_H
#define POD_H

#include <iostream>
#include <string>
#include "Vector.hpp"

struct PodControl {
  int x;
  int y;
  std::string action;
};

class Pod {
 public:
  void WritePodState(std::ostream& output) const;
  void SetTurnConditions(PodControl const& control, int& boost_remaining);
  void EndTurn();
  void MakeProgress(double dt, unsigned int checkpoint_count);
  void Advance(double dt);
  void SetPosition(Vector const& origin, Vector const& direction, double magnitude);
  void PointAt(Vector const& at);

  bool made_progress() const { return made_progress_; }
  double progress_time() const { return progress_time_; }
  bool has_won() const { return lap_ >= 3 && target_checkpoint_ == 1; }
  unsigned int next_checkpoint() const { return target_checkpoint_; }
  Vector const& position() const { return position_; }
  Vector const& velocity() const { return velocity_; }

  static void CollidePods(Pod& pod1, Pod& pod2);

 private:
  int GetBoost(PodControl const& control, int& boost_remaining);

  Vector position_;
  Vector velocity_;
  Vector direction_;
  int lap_ = 0;
  unsigned int target_checkpoint_ = 1;
  int shield_cooldown_ = 0;
  int mass_ = 1;
  bool made_progress_ = false;
  double progress_time_ = 0.0;
};

#endif