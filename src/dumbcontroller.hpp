#ifndef DUMBCONTROLLER_HPP
#define DUMBCONTROLLER_HPP

#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "IPlayer.hpp"
#include "input.hpp"
#include "output.hpp"

class DumbController : public IPlayer {
 public:
  DumbController() : boosts_left_(1) {}

  void SetStreams(std::istream& input, std::ostream& output) override {
    input_ = &input;
    output_ = &output;
  };

  void Setup() override { map_data_ = std::make_unique<MapData>(*input_); }

  void Turn() override {
    PodData me1(*input_, 1, Owner::Me);
    PodData me2(*input_, 2, Owner::Me);
    PodData op1(*input_, 1, Owner::Opponent);
    PodData op2(*input_, 2, Owner::Opponent);
    std::cout << "Positions: " << me1.x << " " << me1.y << " " << me2.x << " " << me2.y
              << std::endl;
    std::cout << "Speeds: " << me1.vx << " " << me1.vy << " " << me2.vx << " " << me2.vy
              << std::endl;

    double thrust = 1.0;

    if (boosts_left_ > 0) {
      TakeMoveBoost(*output_, map_data_->checkpoints[me1.next_checkpoint_id].first,
                    map_data_->checkpoints[me1.next_checkpoint_id].second);
      boosts_left_--;
    } else {
      TakeMove(*output_, map_data_->checkpoints[me1.next_checkpoint_id].first,
               map_data_->checkpoints[me1.next_checkpoint_id].second, thrust);
    }
    if (boosts_left_ > 0) {
      TakeMoveBoost(*output_, map_data_->checkpoints[me2.next_checkpoint_id].first,
                    map_data_->checkpoints[me2.next_checkpoint_id].second);
      boosts_left_--;
    } else {
      TakeMove(*output_, map_data_->checkpoints[me2.next_checkpoint_id].first,
               map_data_->checkpoints[me2.next_checkpoint_id].second, thrust);
    }
  }

 private:
  int boosts_left_;
  std::unique_ptr<MapData> map_data_;
  std::istream* input_;
  std::ostream* output_;
};

#endif