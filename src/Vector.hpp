#ifndef VECTOR_H
#define VECTOR_H

class Vector {
 public:
  Vector();
  Vector(Vector const& p);
  Vector(double x, double y);
  Vector(Vector const& from, Vector const& to);

  /* Accessors */
  double x() const { return x_; }
  double y() const { return y_; }
  double Distance(Vector const& other) const;
  double Length() const;
  int ToDegrees() const;
  double Dot(Vector const& other) const;
  float Cross(Vector const& other) const;

  /* Modifiers */
  void Normalize();
  void Truncate();
  void Round();
  void Rotate(double angle);
  void Add(Vector const& other);

  /* Transformative Constructors */
  Vector Perpendicular() const;
  Vector Shift(Vector const& direction, double magnitude) const;
  Vector Scale(double factor) const;

 private:
  double x_;
  double y_;
};

#endif