#include "Vector.hpp"
#include <cmath>

constexpr double pi() { return std::atan(1) * 4; }

Vector::Vector() : x_(0), y_(0) {}
Vector::Vector(Vector const& p) : x_(p.x_), y_(p.y_) {}
Vector::Vector(double x, double y) : x_(x), y_(y) {}
Vector::Vector(Vector const& from, Vector const& to) : x_(to.x_ - from.x_), y_(to.y_ - from.y_) {}

Vector Vector::Perpendicular() const { return Vector(y_, -x_); }
void Vector::Normalize() {
  double length = Length();
  x_ /= length;
  y_ /= length;
}
void Vector::Truncate() {
  x_ = x_ > 0 ? std::floor(x_) : std::ceil(x_);
  y_ = y_ > 0 ? std::floor(y_) : std::ceil(y_);
}
void Vector::Round() {
  x_ = std::round(x_);
  y_ = std::round(y_);
}
double Vector::Length() const { return sqrt(x_ * x_ + y_ * y_); }

Vector Vector::Shift(Vector const& direction, double magnitude) const {
  return Vector(x_ + direction.x_ * magnitude, y_ + direction.y_ * magnitude);
}

int Vector::ToDegrees() const {
  int angle = static_cast<int>(std::atan(y_ / x_) * 180 / pi());

  if (x_ < 0) {
    angle += 180;
  } else if (x_ == 0) {
    if (y_ > 0) {
      angle = 90;
    } else {
      angle = 270;
    }
  }
  if (angle < 0) {
    angle += 360;
  }

  return angle;
}

double Vector::Dot(Vector const& other) const { return (x_ * other.x_ + y_ * other.y_); }

float Vector::Cross(Vector const& other) const { return (x_ * other.y_ - y_ * other.x_); }

void Vector::Rotate(double angle) {
  double x, y;

  x = std::cos(angle) * x_ - std::sin(angle) * y_;
  y = std::sin(angle) * x_ + std::cos(angle) * y_;

  x_ = x;
  y_ = y;
}

void Vector::Add(Vector const& other) {
  x_ += other.x_;
  y_ += other.y_;
}

double Vector::Distance(Vector const& other) const { return Vector(*this, other).Length(); }

Vector Vector::Scale(double factor) const { return Vector(x_ * factor, y_ * factor); }