#ifndef VECTOR_H
#define VECTOR_H

#include <math.h>

class Vec2 {
 public:
  /* Constructors */
  Vec2(double x = 0.0, double y = 0.0) : x_(x), y_(y) {}
  Vec2(Vec2 const& p) : Vec2(p.x_, p.y_) {}

  /* Accessors */
  double x() const { return x_; }
  double y() const { return y_; }
  double Length() const { return sqrt(*this * *this); }
  inline int Degrees() const;

  /* Modifiers */
  inline void Normalize() { *this *= (1.0 / Length()); }
  inline void Truncate();
  inline void Round();
  inline void Rotate(double angle);

  /* Operators */
  inline Vec2 operator*(double scalar) const;
  inline Vec2& operator*=(double scalar);
  inline double operator*(Vec2 const& other) const;
  inline Vec2 operator+(Vec2 const& other) const;
  inline Vec2& operator+=(Vec2 const& other);
  inline Vec2 operator-(Vec2 const& other) const;
  inline Vec2& operator-=(Vec2 const& other);

  /* Utilities */
  static inline double Dot(Vec2 const& left, Vec2 const& right);
  static inline double Cross(Vec2 const& left, Vec2 const& right);
  static inline Vec2 Perpendicular(Vec2 const& to);
  static constexpr double pi() { return std::atan(1) * 4; }
  static double Cap(double a, double limit) {
    if (a > limit) {
      return limit;
    } else if (a < -limit) {
      return -limit;
    } else {
      return a;
    }
  }

 private:
  double x_;
  double y_;
};

int Vec2::Degrees() const {
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

void Vec2::Truncate() {
  x_ = x_ > 0 ? std::floor(x_) : std::ceil(x_);
  y_ = y_ > 0 ? std::floor(y_) : std::ceil(y_);
}

void Vec2::Round() {
  x_ = std::round(x_);
  y_ = std::round(y_);
}

void Vec2::Rotate(double angle) {
  double x, y;
  x = std::cos(angle) * x_ - std::sin(angle) * y_;
  y = std::sin(angle) * x_ + std::cos(angle) * y_;
  x_ = x;
  y_ = y;
}

Vec2 Vec2::operator*(double scalar) const { return Vec2(x_ * scalar, y_ * scalar); }

Vec2& Vec2::operator*=(double scalar) {
  *this = *this * scalar;
  return *this;
}

double Vec2::operator*(Vec2 const& other) const { return x_ * other.x_ + y_ * other.y_; }

Vec2 Vec2::operator+(Vec2 const& other) const { return Vec2(x_ + other.x_, y_ + other.y_); }

Vec2& Vec2::operator+=(Vec2 const& other) {
  *this = *this + other;
  return *this;
}

Vec2 Vec2::operator-(Vec2 const& other) const { return Vec2(x_ - other.x_, y_ - other.y_); }

Vec2& Vec2::operator-=(Vec2 const& other) {
  *this = *this - other;
  return *this;
}

double Vec2::Dot(Vec2 const& left, Vec2 const& right) { return left * right; }

double Vec2::Cross(Vec2 const& left, Vec2 const& right) {
  return (left.x_ * right.y_ - left.y_ * right.x_);
}

Vec2 Vec2::Perpendicular(Vec2 const& to) {
  Vec2 p(to);
  p.Rotate(-pi() / 2);
  return p;
}

#endif