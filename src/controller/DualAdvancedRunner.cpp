#include "DualAdvancedRunner.hpp"

void DualAdvancedRunner::GetNetworkInput(NetworkInput& input, PodData const& pod,
                                         PodData const& ally, PodData const& lead,
                                         PodData const& trail) {
  std::vector<PodData const*> op = {&lead, &trail};
  GetPodInput(input.pod, pod, ally, op);
}

void DualAdvancedRunner::GetPodInput(InputPod& input, PodData const& pod, PodData const& ally,
                                     std::vector<PodData const*> const& op) {
  GetNextCheckpoints(input.next_checkpoints, pod, pod);
  GetVelocity(input.velocity, pod, Vec2(0, 0), PodAngle(pod));
  // GetOtherPod(input.ally_pod, pod, ally);
  // GetOtherPod(input.leading_enemy_pod, pod, *op.at(0));
  // GetOtherPod(input.trailing_enemy_pod, pod, *op.at(1));
}

void DualAdvancedRunner::GetNextCheckpoints(InputCheckpoint* checkpoints, PodData const& from,
                                            PodData const& perspective) {
  for (unsigned int i = 0; i < kNextCheckpoints; ++i) {
    unsigned int index = perspective.next_checkpoint_id + i;
    index %= map_data_->checkpoints.size();
    Vec2 next(map_data_->checkpoints[index].first, map_data_->checkpoints[index].second);
    GetNextCheckpoint(checkpoints[i], next, from);
  }
}

void DualAdvancedRunner::GetNextCheckpoint(InputCheckpoint& checkpoint, Vec2 const& next,
                                           PodData const& from) {
  /* next checkpoint distance */
  Vec2 position(from.x, from.y);
  Vec2 dp = next - position;
  checkpoint.distance = Vec2::Cap(dp.Length() / kMaxDistance, 1.0);

  /* next checkpoint left/right angles */
  checkpoint.direction = NormalizeAngle(dp.Degrees() - PodAngle(from));
}

void DualAdvancedRunner::GetVelocity(InputVelocity& velocity, PodData const& of,
                                     Vec2 const& ref_velocity, double ref_angle) {
  Vec2 v = Vec2(of.vx, of.vy) - ref_velocity;
  velocity.magnitude = Vec2::Cap(v.Length() / kMaxSpeed, 1.0);
  velocity.direction = NormalizeAngle(v.Degrees() - ref_angle);
}

void DualAdvancedRunner::GetOtherPod(InputOtherPod& output, PodData const& perspective,
                                     PodData const& other) {
  GetNextCheckpoint(output.relative_position, Vec2(other.x, other.y), perspective);
  GetVelocity(output.relative_velocity, other, Vec2(perspective.vx, perspective.vy),
              PodAngle(perspective));
  // GetNextCheckpoints(output.next_checkpoints, other, perspective);
}

void DualAdvancedRunner::WriteNetworkOutput(NetworkOutput const& output, PodData const* pod) {
  WritePodOutput(output.pod, *pod);
}

void DualAdvancedRunner::WritePodOutput(OutputPod const& output, PodData const& pod) {
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
  double right_turn = Vec2::Cap(output.direction * 20, 40.0) / 2;

  double new_pod_angle = pod_angle + right_turn;
  double rad_angle = new_pod_angle * Vec2::pi() / 180;
  Vec2 new_direction(std::cos(rad_angle), std::sin(rad_angle));
  new_direction *= kMaxDistance;
  int x, y;
  x = static_cast<int>(new_direction.x());
  y = static_cast<int>(new_direction.y());

  TakeMove(*output_, pod.x + x, pod.y + y, action);
}

double DualAdvancedRunner::NormalizeAngle(double angle) {
  if (angle < -180) {
    angle += 360;
  }
  if (angle > 180) {
    angle -= 360;
  }
  return angle / 180;
}

double DualAdvancedRunner::PodAngle(PodData const& pod) {
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

unsigned int DualAdvancedRunner::GetLeadPod(std::vector<PodTracker const*> pods) {
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