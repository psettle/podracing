#ifndef RUNNERBLOCKER_HPP
#define RUNNERBLOCKER_HPP
#include <memory>
#include "GameIO.hpp"
#include "IPlayer.hpp"
#include "NeuralNetwork.hpp"
#include "TrainedNetworks.hpp"
#include "Vec2.hpp"

class RunnerBlocker : public IPlayer {
 public:
  struct Config {
    double checkpoint_detection_factor = 1.21258;
    double target_point_shield_thresh = 778.627;
    double opponent_distance_shield_thresh = 1509.37;
    double ally_distance_threash = 1879.61;
    double minimum_checkpoint_distance = 1402.08;
    double maximum_shield_speed = 405.628;
    double target_slowdown_distance = 2273.35;
    double target_offset_factor = 3.57026;
  };

  static unsigned int const kNextCheckpoints = 2;
  struct PodTracker {
    PodData input = PodData();
    unsigned int last_checkpoint = 0;
    unsigned int laps = 0;
    unsigned int boost_left = 1;
  };

  struct InputPosition {
    double distance;
    double direction;
  };
  struct InputVelocity {
    double magnitude;
    double direction;
  };

  struct NetworkInput {
    InputPosition next_checkpoints[kNextCheckpoints];
    InputVelocity velocity;
  };
  static unsigned int const kInputCount = sizeof(NetworkInput) / sizeof(double);

  struct NetworkOutput {
    double thrust;
    double direction;
    double should_shield;
    double should_boost;
  };
  static unsigned int const kOutputCount = sizeof(NetworkOutput) / sizeof(double);

  RunnerBlocker(Config const& config) : runner_(advanced_runner), blocker_(config) {}

  void SetStreams(std::istream& input, std::ostream& output) override {
    input_ = &input;
    output_ = &output;
  };

  void Setup() override { map_data_ = std::make_unique<MapData>(*input_); }

  void Turn() override {
    ReadInput();
    int lead_me = GetLeadPod(&pods_[0], true);
    int lead_op = GetLeadPod(&pods_[2], false);
    PodTracker const& lead = (lead_op == -1) ? pods_[2] : pods_[3];

    if (lead_me == 0 || lead_me == -1) {
      ProcessRunner(pods_[0]);
    } else {
      ProcessBlocker(pods_[0], pods_[1], lead);
    }
    if (lead_me == 0 || lead_me == 1) {
      ProcessRunner(pods_[1]);
    } else {
      ProcessBlocker(pods_[1], pods_[0], lead);
    }

    EndInput();
  }

 private:
  void ReadInput();
  void EndInput();

  /* Runner */
  void ProcessRunner(PodTracker& pod);
  void GetRunnerInput(NetworkInput& input, PodData const& pod);
  void WriteNetworkOutput(NetworkOutput const& output, PodTracker& pod);
  void GetNextCheckpoints(InputPosition* checkpoints, PodData const& from,
                          PodData const& perspective);
  void GetPosition(InputPosition& output, Vec2 const& position, PodData const& perspective);
  void GetVelocity(InputVelocity& velocity, PodData const& of, Vec2 const& ref_velocity,
                   double ref_angle);

  /* Blocker */
  void ProcessBlocker(PodTracker& pod, PodTracker const& ally, PodTracker const& lead);

  double PodAngle(PodData const& pod);
  int GetLeadPod(PodTracker const* pods, bool allow_ties);
  static double NormalizeAngle(double angle);
  static double constexpr kMaxDistance = 16000.0;
  bool first_turn_latch_ = true;
  PodTracker pods_[4];
  std::unique_ptr<MapData> map_data_;
  std::istream* input_;
  std::ostream* output_;
  NeuralNetwork runner_;
  Config blocker_;
};

void RunnerBlocker::ReadInput() {
  pods_[0].input = PodData(*input_, 0, Owner::Me);
  pods_[1].input = PodData(*input_, 1, Owner::Me);
  pods_[2].input = PodData(*input_, 0, Owner::Opponent);
  pods_[3].input = PodData(*input_, 1, Owner::Opponent);

  for (auto& pod : pods_) {
    if (pod.input.next_checkpoint_id == 1 && pod.last_checkpoint == 0) {
      pod.laps++;
    }
  }
}

void RunnerBlocker::EndInput() {
  for (auto& pod : pods_) {
    pod.last_checkpoint = pod.input.next_checkpoint_id;
  }
  first_turn_latch_ = false;
}

void RunnerBlocker::ProcessRunner(PodTracker& pod) {
  NeuralNetwork::Activations runner_in(kInputCount);
  NetworkInput* runner_input = reinterpret_cast<NetworkInput*>(runner_in.data());
  GetRunnerInput(*runner_input, pod.input);
  runner_.SetInput(runner_in);
  NeuralNetwork::Activations const* runner_out = &runner_.GetOutput();
  NetworkOutput const* runner_output = reinterpret_cast<NetworkOutput const*>(runner_out->data());
  WriteNetworkOutput(*runner_output, pod);
}

void RunnerBlocker::GetRunnerInput(NetworkInput& input, PodData const& pod) {
  GetNextCheckpoints(input.next_checkpoints, pod, pod);
  GetVelocity(input.velocity, pod, Vec2(0, 0), PodAngle(pod));
}

void RunnerBlocker::WriteNetworkOutput(NetworkOutput const& output, PodTracker& pod) {
  static double constexpr kAbilityThresh = 0.0;
  std::string action;
  if (output.should_boost > kAbilityThresh && output.should_boost > output.should_shield &&
      pod.boost_left > 0) {
    action = "BOOST";
    pod.boost_left--;
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

  double pod_angle = PodAngle(pod.input);
  double right_turn = Vec2::Cap(output.direction * 20, 40.0) / 2;

  double new_pod_angle = pod_angle + right_turn;
  double rad_angle = new_pod_angle * Vec2::pi() / 180;
  Vec2 new_direction(std::cos(rad_angle), std::sin(rad_angle));
  new_direction *= kMaxDistance;
  int x, y;
  x = static_cast<int>(new_direction.x());
  y = static_cast<int>(new_direction.y());

  TakeMove(*output_, pod.input.x + x, pod.input.y + y, action);
}

void RunnerBlocker::GetNextCheckpoints(InputPosition* checkpoints, PodData const& from,
                                       PodData const& perspective) {
  for (unsigned int i = 0; i < kNextCheckpoints; ++i) {
    unsigned int index = perspective.next_checkpoint_id + i;
    index %= map_data_->checkpoints.size();
    Vec2 next(map_data_->checkpoints[index].first, map_data_->checkpoints[index].second);
    GetPosition(checkpoints[i], next, from);
  }
}

void RunnerBlocker::GetPosition(InputPosition& output, Vec2 const& position,
                                PodData const& perspective) {
  /* position distance */
  Vec2 from(perspective.x, perspective.y);
  Vec2 dp = position - from;
  output.distance = Vec2::Cap(dp.Length() / kMaxDistance, 1.0);

  /* position angle */
  output.direction = NormalizeAngle(dp.Degrees() - PodAngle(perspective));
}

void RunnerBlocker::GetVelocity(InputVelocity& velocity, PodData const& of,
                                Vec2 const& ref_velocity, double ref_angle) {
  static double constexpr kMaxSpeed = 1300.0;
  Vec2 v = Vec2(of.vx, of.vy) - ref_velocity;
  velocity.magnitude = Vec2::Cap(v.Length() / kMaxSpeed, 1.0);
  velocity.direction = NormalizeAngle(v.Degrees() - ref_angle);
}

void RunnerBlocker::ProcessBlocker(PodTracker& pod, PodTracker const& ally,
                                   PodTracker const& lead) {
  Vec2 us_pos(pod.input.x, pod.input.y);
  Vec2 them_pos(lead.input.x, lead.input.y);

  /* Find the next checkpoint of lead that we are closer to than lead */
  unsigned int cp_index = lead.input.next_checkpoint_id;
  std::pair<int, int> const* checkpoint = nullptr;
  for (unsigned int i = 0; i < map_data_->checkpoints.size(); ++i) {
    auto const& temp_cp = map_data_->checkpoints.at(cp_index);
    Vec2 vec_cp(temp_cp.first, temp_cp.second);

    double us_distance = (vec_cp - us_pos).Length();
    double them_distance = (vec_cp - them_pos).Length();
    if (us_distance < them_distance * blocker_.checkpoint_detection_factor) {
      checkpoint = &temp_cp;
      break;
    }

    cp_index++;
    cp_index %= map_data_->checkpoints.size();
  }

  if (!checkpoint) {
    /* They are closer to all of their checkpoints than we are, we are way off the course. */
    ProcessRunner(pod);
    return;
  }
  Vec2 cp(checkpoint->first, checkpoint->second);
  Vec2 cp_direction = cp - them_pos;
  double cp_distance = cp_direction.Length();
  cp_direction.Normalize();
  Vec2 target_point;

  /* Want to either be halfway along opponent's checkpoint vector, or on the boundary of the
   * checkpoint, whichever is closer */
  if (cp_distance * 0.5 > blocker_.minimum_checkpoint_distance) {
    target_point = cp - cp_direction * blocker_.minimum_checkpoint_distance;
  } else {
    target_point = cp - cp_direction * (cp_distance / 2);
  }

  /* Check if are already in a good position and moving slowly enough that shielding will be
   * effective. */
  double opponent_dist = (us_pos - them_pos).Length();
  double ally_dist = (us_pos - Vec2(ally.input.x, ally.input.y)).Length();
  double target_dist = (us_pos - target_point).Length();
  if (opponent_dist < blocker_.opponent_distance_shield_thresh &&
      target_dist < blocker_.target_point_shield_thresh &&
      Vec2(pod.input.vx, pod.input.vy).Length() < blocker_.maximum_shield_speed &&
      ally_dist > blocker_.ally_distance_threash) {
    TakeMove(*output_, pod.input.x, pod.input.y, "SHIELD");
    return;
  }

  /* We aren't in a good state to shield keep trying to move towards the target. */
  double thrust;

  Vec2 target_direction = (target_point - us_pos);
  target_direction.Normalize();
  int abs_angle = NormalizeAngle(target_direction.Degrees() - PodAngle(pod.input));
  if (abs_angle > 90) {
    thrust = 0;
  } else {
    thrust = 1.0 - abs_angle / 90.0;

    if (target_dist < blocker_.target_slowdown_distance) {
      thrust *= (target_dist / blocker_.target_slowdown_distance);
    }

    target_point -= Vec2(pod.input.vx, pod.input.vy) * blocker_.target_offset_factor;
  }

  TakeMove(*output_, target_point.x(), target_point.y(), thrust);
}

double RunnerBlocker::PodAngle(PodData const& pod) {
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

double RunnerBlocker::NormalizeAngle(double angle) {
  if (angle < -180) {
    angle += 360;
  }
  if (angle > 180) {
    angle -= 360;
  }
  return angle / 180;
}

int RunnerBlocker::GetLeadPod(PodTracker const* pods, bool allow_ties) {
  if (pods[0].laps < pods[1].laps) {
    return 1;
  }
  if (pods[1].laps < pods[0].laps) {
    return -1;
  }

  if (pods[0].input.next_checkpoint_id == pods[1].input.next_checkpoint_id) {
    if (allow_ties) {
      return 0;
    }

    /* Aiming for same checkpoint, calculate closest pod. */
    auto const& c = map_data_->checkpoints[pods[0].input.next_checkpoint_id];
    Vec2 cp(c.first, c.second);
    Vec2 p0(pods[0].input.x, pods[0].input.y);
    Vec2 p1(pods[1].input.x, pods[1].input.y);
    p0 -= cp;
    p1 -= cp;
    if (p0.Length() < p1.Length()) {
      return -1;
    } else {
      return 1;
    }
  } else if (pods[0].input.next_checkpoint_id < pods[1].input.next_checkpoint_id) {
    if (pods[0].input.next_checkpoint_id == 0) {
      return -1;
    } else {
      return 1;
    }
  } else {
    if (pods[1].input.next_checkpoint_id == 0) {
      return 1;
    } else {
      return -1;
    }
  }
}

#endif