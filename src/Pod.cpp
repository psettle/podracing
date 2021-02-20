#include "Pod.hpp"
#include <cmath>

void Pod::WritePodState(std::ostream& output) const {
  output << static_cast<int>(position_.x()) << " " << static_cast<int>(position_.y()) << " "
         << static_cast<int>(velocity_.x()) << " " << static_cast<int>(velocity_.y()) << " "
         << direction_.ToDegrees() << " " << target_checkpoint_ << std::endl;
}

void Pod::SetTurnConditions(PodControl const& control, int& boosts_available) {
  /* Figure out current thrust value */
  int boost = GetBoost(control, boosts_available);
  if (shield_cooldown_ > 0) {
    shield_cooldown_--;
  }

  /* Turn vehicle as much as allowed */
  Vector desired_direction(position_, Vector(control.x, control.y));
  desired_direction.Normalize();
  double dot = direction_.Dot(desired_direction);
  if (dot > 1.0) {
    dot = 1.0;
  } else if (dot < -1.0) {
    dot = -1.0;
  }
  double da = std::acos(dot);
  if (da < 0.314159) {
    direction_ = desired_direction;
  } else {
    double cross = direction_.Cross(desired_direction);
    direction_.Rotate(cross > 0 ? 0.314159 : -0.314159);
  }

  /* Update speed by boost */
  velocity_.Add(direction_.Scale(boost));

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
  velocity_ = velocity_.Scale(0.85);

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

void Pod::Advance(double dt) { position_.Add(velocity_.Scale(dt)); }

void Pod::SetPosition(Vector const& origin, Vector const& direction, double magnitude) {
  position_ = origin.Shift(direction, magnitude);
  position_.Round();
}

void Pod::PointAt(Vector const& at) {
  direction_ = Vector(position_, at);
  direction_.Normalize();
}

void Pod::CollidePods(Pod& pod1, Pod& pod2) {
  Vector dp(pod1.position_, pod2.position_);
  Vector dv(pod1.velocity_, pod2.velocity_);
  double m =
      static_cast<double>(pod1.mass_ + pod2.mass_) / static_cast<double>(pod1.mass_ * pod2.mass_);

  float seperation2 = dp.Length() * dp.Length();
  float product = dp.Dot(dv);

  Vector f = dp.Scale(product / (seperation2 * m));

  pod1.velocity_.Add(f.Scale(1.0 / pod1.mass_));
  pod2.velocity_.Add(f.Scale(-1.0 / pod2.mass_));

  float impulse = f.Length();
  if (impulse < 120.0) {
    f = f.Scale(120.0 / impulse);
  }

  pod1.velocity_.Add(f.Scale(1.0 / pod1.mass_));
  pod2.velocity_.Add(f.Scale(-1.0 / pod2.mass_));
}