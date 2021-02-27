
#ifndef DUALADVANCEDRUNNER_HPP
#define DUALADVANCEDRUNNER_HPP

#include <math.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include "GameIO.hpp"
#include "IPlayer.hpp"
#include "NeuralNetwork.hpp"
#include "Vec2.hpp"

class DualAdvancedRunner : public IPlayer {
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

  DualAdvancedRunner(NeuralNetwork& core) : boosts_left_(1), network_(core) {}

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

#endif