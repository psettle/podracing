#include "Pod.hpp"
#include <cmath>
#include <vector>

void Pod::WritePodState(std::ostream& output) const {
  output << static_cast<int>(position_.x()) << " " << static_cast<int>(position_.y()) << " "
         << static_cast<int>(velocity_.x()) << " " << static_cast<int>(velocity_.y()) << " "
         << direction_.Degrees() << " " << target_checkpoint_ << std::endl;
}

void Pod::SetTurnConditions(PodControl const& control, int& boosts_available, bool first_frame) {
  static double const kMaxAngle = Vec2::pi() / 10;

  /* Figure out current thrust value */
  int boost = GetBoost(control, boosts_available);
  if (shield_cooldown_ > 0) {
    shield_cooldown_--;
  }

  /* Turn vehicle as much as allowed */
  Vec2 desired_direction = Vec2(control.x, control.y) - position_;
  desired_direction.Normalize();
  double dot = Vec2::Cap(Vec2::Dot(direction_, desired_direction), 1.0);

  double angle = std::acos(dot);
  if (angle < kMaxAngle || first_frame) {
    direction_ = desired_direction;
  } else {
    double cross = Vec2::Cross(direction_, desired_direction);
    direction_.Rotate(cross > 0 ? kMaxAngle : -kMaxAngle);
  }

  /* Update speed by boost */
  velocity_ += direction_ * boost;

  made_progress_ = false;
  progress_time_ = 0.0;
}

int Pod::GetBoost(PodControl const& control, int& boosts_available) {
  if (control.action == "SHIELD") {
    mass_ = 10;
    shield_cooldown_ = 4;
  } else {
    mass_ = 1;
  }
  if (shield_cooldown_ > 0) {
    return 0;
  }
  if (control.action == "BOOST") {
    if (boosts_available > 0) {
      boosts_available--;
      return 650;
    }
    return 100;
  }
  return std::atoi(control.action.c_str());
}

void Pod::EndTurn() {
  /* Apply friction */
  velocity_ *= 0.85;
  velocity_.Truncate();
  position_.Round();
}

void Pod::MakeProgress(double dt, unsigned int checkpoint_count) {
  made_progress_ = true;
  progress_time_ = dt;

  target_checkpoint_++;
  if (target_checkpoint_ >= checkpoint_count) {
    target_checkpoint_ = 0;
    lap_++;
  }
}

void Pod::Advance(double dt) { position_ += velocity_ * dt; }

void Pod::SetPosition(Vec2 const& origin, Vec2 const& direction, double magnitude) {
  position_ = origin + direction * magnitude;
  position_.Round();
}

void Pod::PointAt(Vec2 const& at) {
  direction_ = at - position_;
  direction_.Normalize();
}

void Pod::CollidePods(Pod& pod1, Pod& pod2) {
  Vec2 dp = pod2.position_ - pod1.position_;
  Vec2 dv = pod2.velocity_ - pod1.velocity_;
  double m =
      static_cast<double>(pod1.mass_ + pod2.mass_) / static_cast<double>(pod1.mass_ * pod2.mass_);

  double seperation2 = dp.Length() * dp.Length();
  double product = Vec2::Dot(dp, dv);

  Vec2 f = dp * (product / (seperation2 * m));

  pod1.velocity_ += f * (1.0 / pod1.mass_);
  pod2.velocity_ -= f * (1.0 / pod2.mass_);

  float impulse = f.Length();
  if (impulse < 120.0) {
    f *= (120.0 / impulse);
  }

  pod1.velocity_ += f * (1.0 / pod1.mass_);
  pod2.velocity_ -= f * (1.0 / pod2.mass_);
}

double Pod::GetFitness(std::vector<Vec2> const& map) const {
  /* Number of checkpoints that need to be hit */
  double max_fitness = (3 * map.size() + 1);
  double fitness = max_fitness;

  /* Number of checkpoints actually hit */
  fitness -= (lap_ * map.size() + target_checkpoint_);

  if (fitness > 0) {
    /* Add distance to next checkpoint */
    unsigned int prev_checkpoint = target_checkpoint_;
    if (prev_checkpoint > 0) {
      prev_checkpoint--;
    } else {
      prev_checkpoint = map.size() - 1;
    }
    // Vec2 segment = map.at(target_checkpoint_) - map.at(prev_checkpoint);

    // Vec2 distance = map.at(target_checkpoint_) - position_;

    // fitness += distance.Length() / segment.Length();
  }

  return fitness / max_fitness;
}