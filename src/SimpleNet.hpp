#ifndef SIMPLENET_HPP
#define SIMPLENET_HPP

#include <math.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include "IPlayer.hpp"
#include "NeuralNetwork.hpp"
#include "Vec2.hpp"
#include "input.hpp"
#include "output.hpp"

class SimpleNet : public IPlayer {
 public:
  enum NetworkInput {
    INPUT_NEXT_CHECKPOINT_DISTANCE,
    INPUT_NEXT_CHECKPOINT_DIRECTION,
    INPUT_VELOCITY_MAGNITUDE,
    INPUT_VELOCITY_DIRECTION,

    INPUT_COUNT
  };

  enum NetworkOutput {
    OUTPUT_THRUST,
    OUTPUT_DIRECTION,
    OUTPUT_SHIELD,
    OUTPUT_BOOST,

    OUTPUT_COUNT
  };

  SimpleNet(NeuralNetwork& core) : boosts_left_(1), network_(core) {}

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

    NeuralNetwork::Activations const* output;

    network_.SetInput(GetNetworkInput(me1));
    output = &network_.GetOutput();
    WriteNetworkOutput(me1, *output);

    network_.SetInput(GetNetworkInput(me2));
    output = &network_.GetOutput();
    WriteNetworkOutput(me2, *output);

    first_turn_latch_ = false;
  }

 private:
  static double constexpr kMaxDistance = 16000.0;
  static double constexpr kMaxSpeed = 1300.0;

  NeuralNetwork::Activations GetNetworkInput(PodData const& pod) {
    NeuralNetwork::Activations input(INPUT_COUNT);

    /* next checkpoint distance */
    auto const& next_checkpoint = map_data_->checkpoints[pod.next_checkpoint_id];
    Vec2 next_cp(next_checkpoint.first, next_checkpoint.second);
    Vec2 position(pod.x, pod.y);
    Vec2 dp = next_cp - position;
    input[INPUT_NEXT_CHECKPOINT_DISTANCE] = Vec2::Cap(dp.Length() / kMaxDistance, 1.0);

    /* next checkpoint left/right angles */
    double pod_angle = pod.angle;
    if (first_turn_latch_) {
      pod_angle = dp.Degrees();
    }
    double next_cp_angle = dp.Degrees();
    double right_cp_angle = next_cp_angle - pod_angle;
    if (right_cp_angle < -180) {
      right_cp_angle += 360;
    }
    if (right_cp_angle > 180) {
      right_cp_angle -= 360;
    }
    input[INPUT_NEXT_CHECKPOINT_DIRECTION] = right_cp_angle / 180.0;
    /* velocity */
    Vec2 velocity = Vec2(pod.vx, pod.vy);
    input[INPUT_VELOCITY_MAGNITUDE] = Vec2::Cap(velocity.Length() / kMaxSpeed, 1.0);

    /* velocity direction */
    double next_velocity_angle = velocity.Degrees();
    double right_velocity_angle = next_velocity_angle - pod_angle;
    if (velocity.Length() == 0.0) {
      right_velocity_angle = 0.0;
    }
    if (right_velocity_angle < -180) {
      right_velocity_angle += 360;
    }
    if (right_velocity_angle > 180) {
      right_velocity_angle -= 360;
    }
    input[INPUT_VELOCITY_DIRECTION] = right_velocity_angle / 180.0;

    return input;
  }

  static double constexpr kAbilityThresh = 0.5;
  void WriteNetworkOutput(PodData const& pod, NeuralNetwork::Activations const& output) {
    std::string action;
    if (output[OUTPUT_BOOST] > kAbilityThresh && output[OUTPUT_BOOST] > output[OUTPUT_SHIELD] &&
        boosts_left_ > 0) {
      action = "BOOST";
      boosts_left_--;
    } else if (output[OUTPUT_SHIELD] > kAbilityThresh &&
               output[OUTPUT_SHIELD] > output[OUTPUT_BOOST]) {
      action = "SHIELD";
    } else {
      int thrust = static_cast<int>(output[OUTPUT_THRUST] * 100);
      if (thrust > 100) {
        thrust = 100;
      } else if (thrust < 0) {
        thrust = 0;
      }
      action = std::to_string(thrust);
    }

    double right_turn = Vec2::Cap(output[OUTPUT_DIRECTION], 1.0) * 20;
    double pod_angle = pod.angle;
    if (first_turn_latch_) {
      auto const& next_checkpoint = map_data_->checkpoints[pod.next_checkpoint_id];
      Vec2 next_cp(next_checkpoint.first, next_checkpoint.second);
      Vec2 position(pod.x, pod.y);
      Vec2 dp = next_cp - position;
      pod_angle = dp.Degrees();
    }
    double new_pod_angle = pod_angle + right_turn;
    double rad_angle = new_pod_angle * Vec2::pi() / 180;
    Vec2 new_direction(std::cos(rad_angle), std::sin(rad_angle));
    new_direction *= kMaxDistance;
    int x, y;
    x = static_cast<int>(new_direction.x());
    y = static_cast<int>(new_direction.y());

    // std::cerr << "b: " << output[OUTPUT_BOOST] << " l: " << output[OUTPUT_LEFT_STEER]
    //           << " r: " << output[OUTPUT_RIGHT_STEER];
    TakeMove(*output_, pod.x + x, pod.y + y, action);
  }

  int boosts_left_;
  bool first_turn_latch_ = true;
  std::unique_ptr<MapData> map_data_;
  std::istream* input_;
  std::ostream* output_;
  NeuralNetwork& network_;
};

#endif
