#ifndef NEURALCONTROLLER_HPP
#define NEURALCONTROLLER_HPP

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

class TunedController : public IPlayer {
 public:
  struct Config {};
  struct PodTracker {
    PodData input = PodData();
    unsigned int last_checkpoint = 0;
    unsigned int laps = 0;
  };

  TunedController(Config const& config) : boosts_left_(1), config_(config) {}

  void SetStreams(std::istream& input, std::ostream& output) override {
    input_ = &input;
    output_ = &output;
  };

  void Setup() override { map_data_ = std::make_unique<MapData>(*input_); }

  void Turn() override {
    ReadInput();
    std::vector<PodTracker const*> me = {&pods_[0], &pods_[1]};
    std::vector<PodTracker const*> op = {&pods_[2], &pods_[3]};
    unsigned int lead_me_pod = GetLeadPod(me);
    unsigned int lead_op_pod = GetLeadPod(op);
    PodTracker const& op_lead = lead_op_pod == 0 ? pods_[2] : pods_[3];
    PodTracker const& op_trail = lead_op_pod == 1 ? pods_[2] : pods_[3];

    if (0 == lead_me_pod) {
      ProcessLeadPod(*me.at(0), op_trail);
      ProcessTrailPod(*me.at(1), op_lead);
    } else {
      ProcessTrailPod(*me.at(0), op_lead);
      ProcessLeadPod(*me.at(1), op_trail);
    }

    EndInput();
  }

 private:
  void ReadInput() {
    pods_[0].input = PodData(*input_, 0, Owner::Me);
    pods_[1].input = PodData(*input_, 1, Owner::Me);
    pods_[2].input = PodData(*input_, 0, Owner::Opponent);
    pods_[3].input = PodData(*input_, 1, Owner::Opponent);

    for (auto& pod : pods_) {
      if (pod.input.next_checkpoint_id < pod.last_checkpoint) {
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

  void ProcessLeadPod(PodTracker const& pod, PodTracker const& trail) const;

  void ProcessTrailPod(PodTracker const& pod, PodTracker const& lead) const;

  unsigned int GetLeadPod(std::vector<PodTracker const*> pods);
  static double NormalizeAngle(double angle);
  double PodAngle(PodData const& pod) const;

  int boosts_left_;
  bool first_turn_latch_ = true;
  PodTracker pods_[4];
  std::unique_ptr<MapData> map_data_;
  std::istream* input_;
  std::ostream* output_;
  Config config_;
};

double TunedController::NormalizeAngle(double angle) {
  if (angle < -180) {
    angle += 360;
  }
  if (angle > 180) {
    angle -= 360;
  }
  return angle / 180;
}

double TunedController::PodAngle(PodData const& pod) const {
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

unsigned int TunedController::GetLeadPod(std::vector<PodTracker const*> pods) {
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

void TunedController::ProcessLeadPod(PodTracker const& pod, PodTracker const& trail) const {
  auto const& next_checkpoint = map_data_->checkpoints[pod.input.next_checkpoint_id];
  Vec2 cp(next_checkpoint.first, next_checkpoint.second);
  Vec2 position(pod.input.x, pod.input.y);
  Vec2 dp = cp - position;
  double cp_angle = std::abs(dp.Degrees() - PodAngle(pod.input));
  double cp_distance = dp.Length();

  int thrust;
  bool boost = false;
  int x = cp.x();
  int y = cp.y();
  if (cp_angle > 90) {
    thrust = 0;
  } else {
    thrust = 1.0 - cp_angle / 90.0;

    if (thrust > 0.9 && boosts_left_ > 0 && cp_distance > 5000) {
      boost = true;
    }

    if (cp_distance < 1200) {
      thrust *= (cp_distance / 1200.0);
    }

    x -= 3 * pod.input.vx;
    y -= 3 * pod.input.vy;
  }

  // next cp inf = f(cp dist, cp speed, cp angle)
  // goal = (cp) * (1 - next cp inf) + (next cp) * (next cp inf)
  // aim = goal + f(speed, goal distance)
  // thrust = f(aim angle, aim distance)

  // f(x, y, z) { (a * x ^ 2 + b * x + c) * (a * y ^ 2 + b * y + c) * (a * z ^ 2 + b * z + c)  }

  if (boost) {
    TakeMoveBoost(*output_, x, y);
  } else {
    TakeMove(*output_, x, y, thrust);
  }
}
void TunedController::ProcessTrailPod(PodTracker const& pod, PodTracker const& lead) const {
  ProcessLeadPod(pod, lead);
}

#endif
