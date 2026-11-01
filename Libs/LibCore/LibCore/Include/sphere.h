// Sphere

#pragma once

#include "circle.h"
#include "random.h"

// Sphere class
class Sphere
{
protected:
  Vector c = Vector::Null; //!< Center.
  double r = 0.0; //!< Radius.
public:
  //! Empty.
  Sphere() {}
  explicit Sphere(const double&);
  explicit Sphere(const Vector&, const double& = 0.0);
  explicit Sphere(const Vector&, const Vector&);
  explicit Sphere(const Vector&, const Vector&, const Vector&);
  explicit Sphere(const Vector&, const Vector&, const Vector&, const Vector&);
  explicit Sphere(const QVector<Vector>&);

  //! Empty.
  ~Sphere() {}

  // Parameters
  Vector Center() const;
  double Radius() const;

  Box GetBox() const;

  // Ray intersection
  bool Intersect(const Ray&) const;
  int Intersect(const Ray&, double&, double&) const;
  int Intersect(const Ray&, double&, double&, Vector&, Vector&) const;

  // Shape intersection
  bool Intersect(const Box&) const;
  bool Intersect(const Sphere&, Circle&) const;
  bool Intersect(const Sphere&) const;
  bool SurfaceIntersect(const Sphere&) const;

  bool Intersect(const Ray&, double&) const;
  bool Inside(const Vector&) const;

  // Volume and area
  double Volume() const;
  double Volume(const Sphere&) const;

  double Area() const;

  Vector Normal(const Vector&) const;

  // Squared distance to a sphere
  double R(const Vector&) const;
  double Signed(const Vector&) const;

  // Signed distance between two spheres
  double R(const Sphere&) const;

  // Geodesic distance between two points on a sphere
  double R(const Vector&, const Vector&) const;

  void Rotate(const Matrix&);
  void Translate(const Vector&);
  void Scale(const double&);

  Sphere Translated(const Vector&) const;
  Sphere Scaled(const Vector&) const;
  Sphere Rotated(const Matrix&) const;
  Sphere Transformed(const Frame&) const;
  Sphere InverseTransformed(const Frame&) const;

  void Extend(const Vector&);

  // Minkowski sum
  friend Sphere operator+(const Sphere&, const Sphere&);
  friend Sphere operator*(const double&, const Sphere&);
  friend Sphere operator*(const Sphere&, const double&);

  void Extend(const double&);
  Sphere Extended(const double&) const;

  // Random
  Vector RandomSurface(Random & = Random::R239) const;
  Vector RandomInside(Random & = Random::R239) const;
  static Vector RandomNormal(Random & = Random::R239);

  static double Area(const double&);
  static double Volume(const double&);

  Vector Fibonacci(int, int) const;
  QVector<Vector> Poisson(const double&, int, Random & = Random::R239) const;

  Vector2 Euler(const Vector&) const;
  static Vector2 EquiRectangular(int, int, int, int);
  static bool Intersection(const Sphere&, const Sphere&, const Sphere&, Vector&, Vector&);
  static bool Intersection(const Sphere&, const Sphere&, const Vector&, Vector&, Vector&);

  friend std::ostream& operator<<(std::ostream&, const Sphere&);
public:
  static const double epsilon; //!< \f$ \varepsilon \f$ for intersection tests.
  static const Sphere Null; //!< Empty sphere.
  static const Sphere Infinity; //!< Infinite sphere.
  static const Sphere Unit; //!< %Unit sphere.
};

//! Gets the center of a sphere.
inline Vector Sphere::Center() const
{
  return c;
}

//! Gets the radius of a sphere.
inline double Sphere::Radius() const
{
  return r;
}








