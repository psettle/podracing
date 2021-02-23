
#ifndef HCNEURAL_HPP
#define HCNEURAL_HPP

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

class HCNeural : public IPlayer {
 public:
  static unsigned int const kNextCheckpoints = 2;
  struct PodTracker {
    PodData input = PodData();
    unsigned int last_checkpoint = 0;
    unsigned int laps = 0;
  };
  struct InputCheckpoint {
    double distance;
    double direction;
  };
  struct InputVelocity {
    double magnitude;
    double direction;
  };
  struct InputOtherPod {
    InputCheckpoint relative_position;
    InputVelocity relative_velocity;
    // InputCheckpoint next_checkpoints[kNextCheckpoints];
  };
  struct InputPod {
    InputCheckpoint next_checkpoints[kNextCheckpoints];
    InputVelocity velocity;
    // InputOtherPod ally_pod;
    // InputOtherPod leading_enemy_pod;
    // InputOtherPod trailing_enemy_pod;
  };
  struct NetworkInput {
    InputPod pod;
  };
  struct OutputPod {
    double thrust;
    double direction;
    double should_shield;
    double should_boost;
  };
  struct NetworkOutput {
    OutputPod pod;
  };
  static unsigned int const kInputCount = sizeof(NetworkInput) / sizeof(double);
  static unsigned int const kOutputCount = sizeof(NetworkOutput) / sizeof(double);

  HCNeural(NeuralNetwork& core) : boosts_left_(1), network_(core) {}

  void SetStreams(std::istream& input, std::ostream& output) override {
    input_ = &input;
    output_ = &output;
  };

  void Setup() override { map_data_ = std::make_unique<MapData>(*input_); }

  void Turn() override {
    ReadInput();
    std::vector<PodTracker const*> me = {&pods_[0], &pods_[1]};
    std::vector<PodTracker const*> op = {&pods_[2], &pods_[3]};
    unsigned int lead_op_pod = GetLeadPod(op);
    PodTracker const& op_lead = lead_op_pod == 0 ? pods_[2] : pods_[3];
    PodTracker const& op_trail = lead_op_pod == 1 ? pods_[2] : pods_[3];

    NeuralNetwork::Activations in(kInputCount);
    NetworkInput* input = reinterpret_cast<NetworkInput*>(in.data());
    GetNetworkInput(*input, pods_[0].input, pods_[1].input, op_lead.input, op_trail.input);
    network_.SetInput(in);
    NeuralNetwork::Activations const* out0 = &network_.GetOutput();
    NetworkOutput const* output = reinterpret_cast<NetworkOutput const*>(out0->data());
    WriteNetworkOutput(*output, &me[0]->input);

    input = reinterpret_cast<NetworkInput*>(in.data());
    GetNetworkInput(*input, pods_[1].input, pods_[0].input, op_lead.input, op_trail.input);
    network_.SetInput(in);
    out0 = &network_.GetOutput();
    output = reinterpret_cast<NetworkOutput const*>(out0->data());
    WriteNetworkOutput(*output, &me[1]->input);

    EndInput();
  }

 private:
  void ReadInput() {
    pods_[0].input = PodData(*input_, 0, Owner::Me);
    pods_[1].input = PodData(*input_, 1, Owner::Me);
    pods_[2].input = PodData(*input_, 0, Owner::Opponent);
    pods_[3].input = PodData(*input_, 1, Owner::Opponent);

    for (auto& pod : pods_) {
      if (pod.input.next_checkpoint_id < static_cast<int>(pod.last_checkpoint)) {
        pod.last_checkpoint++;
      }
    }
  }

  void EndInput() {
    for (auto& pod : pods_) {
      pod.last_checkpoint = pod.input.next_checkpoint_id;
    }
    first_turn_latch_ = false;
  }

  static double constexpr kMaxDistance = 16000.0;
  static double constexpr kMaxSpeed = 1300.0;

  void GetNetworkInput(NetworkInput& input, PodData const& pod, PodData const& ally,
                       PodData const& lead, PodData const& trail);
  void GetPodInput(InputPod& input, PodData const& pod, PodData const& ally,
                   std::vector<PodData const*> const& op);

  static double constexpr kAbilityThresh = 0.0;
  void WriteNetworkOutput(NetworkOutput const& output, PodData const* pod);
  void WritePodOutput(OutputPod const& output, PodData const& pod);

  static double NormalizeAngle(double angle);
  double PodAngle(PodData const& pod);

  void GetNextCheckpoints(InputCheckpoint* checkpoints, PodData const& from,
                          PodData const& perspective);

  void GetNextCheckpoint(InputCheckpoint& checkpoint, Vec2 const& next, PodData const& from);
  void GetVelocity(InputVelocity& velocity, PodData const& of, Vec2 const& ref_velocity,
                   double ref_angle);
  void GetOtherPod(InputOtherPod& output, PodData const& perspective, PodData const& other);
  unsigned int GetLeadPod(std::vector<PodTracker const*> pods);

  int boosts_left_;
  bool first_turn_latch_ = true;
  PodTracker pods_[4];
  std::unique_ptr<MapData> map_data_;
  std::istream* input_;
  std::ostream* output_;
  NeuralNetwork& network_;
};

void HCNeural::GetNetworkInput(NetworkInput& input, PodData const& pod, PodData const& ally,
                               PodData const& lead, PodData const& trail) {
  std::vector<PodData const*> op = {&lead, &trail};
  GetPodInput(input.pod, pod, ally, op);
}

void HCNeural::GetPodInput(InputPod& input, PodData const& pod, PodData const& ally,
                           std::vector<PodData const*> const& op) {
  GetNextCheckpoints(input.next_checkpoints, pod, pod);
  GetVelocity(input.velocity, pod, Vec2(0, 0), PodAngle(pod));
  // GetOtherPod(input.ally_pod, pod, ally);
  // GetOtherPod(input.leading_enemy_pod, pod, *op.at(0));
  // GetOtherPod(input.trailing_enemy_pod, pod, *op.at(1));
}

void HCNeural::GetNextCheckpoints(InputCheckpoint* checkpoints, PodData const& from,
                                  PodData const& perspective) {
  for (unsigned int i = 0; i < kNextCheckpoints; ++i) {
    unsigned int index = perspective.next_checkpoint_id + i;
    index %= map_data_->checkpoints.size();
    Vec2 next(map_data_->checkpoints[index].first, map_data_->checkpoints[index].second);
    GetNextCheckpoint(checkpoints[i], next, from);
  }
}

void HCNeural::GetNextCheckpoint(InputCheckpoint& checkpoint, Vec2 const& next,
                                 PodData const& from) {
  /* next checkpoint distance */
  Vec2 position(from.x, from.y);
  Vec2 dp = next - position;
  checkpoint.distance = Vec2::Cap(dp.Length() / kMaxDistance, 1.0);

  /* next checkpoint left/right angles */
  checkpoint.direction = NormalizeAngle(dp.Degrees() - PodAngle(from));
}

void HCNeural::GetVelocity(InputVelocity& velocity, PodData const& of, Vec2 const& ref_velocity,
                           double ref_angle) {
  Vec2 v = Vec2(of.vx, of.vy) - ref_velocity;
  velocity.magnitude = Vec2::Cap(v.Length() / kMaxSpeed, 1.0);
  velocity.direction = NormalizeAngle(v.Degrees() - ref_angle);
}

void HCNeural::GetOtherPod(InputOtherPod& output, PodData const& perspective,
                           PodData const& other) {
  GetNextCheckpoint(output.relative_position, Vec2(other.x, other.y), perspective);
  GetVelocity(output.relative_velocity, other, Vec2(perspective.vx, perspective.vy),
              PodAngle(perspective));
  // GetNextCheckpoints(output.next_checkpoints, other, perspective);
}

void HCNeural::WriteNetworkOutput(NetworkOutput const& output, PodData const* pod) {
  WritePodOutput(output.pod, *pod);
}

void HCNeural::WritePodOutput(OutputPod const& output, PodData const& pod) {
  std::string action;
  if (output.should_boost > kAbilityThresh && output.should_boost > output.should_shield &&
      boosts_left_ > 0) {
    action = "BOOST";
    boosts_left_--;
  } else if (output.should_shield > kAbilityThresh && output.should_shield > output.should_boost) {
    action = "SHIELD";
  } else {
    int thrust = static_cast<int>(output.thrust * 120);
    if (thrust > 100) {
      thrust = 100;
    } else if (thrust < 0) {
      thrust = 0;
    }
    action = std::to_string(thrust);
  }

  double pod_angle = PodAngle(pod);
  double right_turn = Vec2::Cap(output.direction * 20, 18.0);

  double new_pod_angle = pod_angle + right_turn;
  double rad_angle = new_pod_angle * Vec2::pi() / 180;
  Vec2 new_direction(std::cos(rad_angle), std::sin(rad_angle));
  new_direction *= kMaxDistance;
  int x, y;
  x = static_cast<int>(new_direction.x());
  y = static_cast<int>(new_direction.y());

  TakeMove(*output_, pod.x + x, pod.y + y, action);
}

double HCNeural::NormalizeAngle(double angle) {
  if (angle < -180) {
    angle += 360;
  }
  if (angle > 180) {
    angle -= 360;
  }
  return angle / 180;
}

double HCNeural::PodAngle(PodData const& pod) {
  double pod_angle = pod.angle;
  if (first_turn_latch_) {
    /* On the first turn the game always tells us we are pointing at 0,
       then actually lets us do an instant rotation to the first angle we request.
       We replace the angle provided by the game with the angle to the first checkpoint
       on the first turn. */
    auto const& next_checkpoint = map_data_->checkpoints[pod.next_checkpoint_id];
    Vec2 next_cp(next_checkpoint.first, next_checkpoint.second);
    Vec2 position(pod.x, pod.y);
    Vec2 dp = next_cp - position;
    pod_angle = dp.Degrees();
  }
  return pod_angle;
}

unsigned int HCNeural::GetLeadPod(std::vector<PodTracker const*> pods) {
  if (pods[0]->laps < pods[1]->laps) {
    return 1;
  }
  if (pods[1]->laps < pods[0]->laps) {
    return 0;
  }
  if (pods[0]->input.next_checkpoint_id < pods[1]->input.next_checkpoint_id) {
    return 1;
  }
  if (pods[1]->input.next_checkpoint_id < pods[0]->input.next_checkpoint_id) {
    return 0;
  }
  /* Aiming for same checkpoint, calculate closest pod. */
  auto const& c = map_data_->checkpoints[pods[0]->input.next_checkpoint_id];
  Vec2 cp(c.first, c.second);
  Vec2 p0(pods[0]->input.x, pods[0]->input.y);
  Vec2 p1(pods[1]->input.x, pods[1]->input.y);

  if (Vec2::Dot(p0, p0) < Vec2::Dot(p1, p1)) {
    return 0;
  } else {
    return 1;
  }
}

#endif