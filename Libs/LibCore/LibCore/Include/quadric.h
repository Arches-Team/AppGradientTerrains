// Quadric polynomials

#pragma once

#include "linear.h"
#include <QtCore/QVector>

class Quadric {
protected:
  double c[3] = { 0.0,0.0,0.0 }; //!< %Array of coefficients.
public:
  //! Empty.
  Quadric() {}
  explicit Quadric(const double&, const double&, const double&);
  explicit Quadric(const Linear&);
  //! Empty.
  ~Quadric() {}

  constexpr double& operator[] (int);
  constexpr double operator[] (int) const;

  // Unary operators
  Quadric operator+ () const { return *this; }
  Quadric operator- () const;

  Quadric& operator+= (const Quadric&);
  Quadric& operator-= (const Quadric&);
  Quadric& operator*= (const double&);
  Quadric& operator/= (const double&);

  // Binary operators
  friend Quadric operator+ (const Quadric&, const Quadric&);
  friend Quadric operator- (const Quadric&, const Quadric&);

  friend Quadric operator* (const Quadric&, const double&);
  friend Quadric operator* (const double&, const Quadric&);
  friend Quadric operator/ (const Quadric&, const double&);
  friend Quadric operator* (const Linear&, const Linear&);
  Quadric operator+(const double&) const;

  // Evaluates polynomial
  constexpr double operator()(const double&) const;
  constexpr double Derivative(const double&) const;

  // Derivative
  Linear Prime() const;

  // Solve
  int Solve(double&, double&) const;
  int Solve(double*) const;
  int Solve(double*, const double&, const double&) const;

  void Range(double&, double&, const double& = 0.0, const double& = 1.0) const;
  double Maximum(const double& = 0.0, const double& = 1.0) const;
  double Minimum(const double& = 0.0, const double& = 1.0) const;

  // Lipschitz
  double K(const double&, const double&) const;

  static Quadric Bezier(const double&, const double&, const double&);

  static double Smooth(const double&, const double&);
  static double Smooth(const double&);
  static double SmoothCompact(const double&, const double&);
  static double SmoothCompact(const double&, const double&, const double&);

  static double Warp(const double&);

  // Bernstein
  static Quadric Bernstein(int);
  static double Bernstein(int, const double&);

  static Quadric FromRoots(const QVector<double>&);

  friend std::ostream& operator<<(std::ostream&, const Quadric&);
public:
  static const double epsilon; //!< Epsilon value used to check discriminant term in the root finding process.
};

//! Creates a quadric.
inline Quadric::Quadric(const double& a, const double& b, const double& c) :c{c,b,a}
{
}


//! Creates a quadric from a Linear polynomial.
inline Quadric::Quadric(const Linear& l) : c{ l[0],l[1],0.0 }
{
}

//! Access class components.
inline constexpr double& Quadric::operator[] (int i)
{
  return c[i];
}

//! Overloaded.
inline constexpr double Quadric::operator[] (int i) const
{
  return c[i];
}


//! Evaluates the quadric. 
inline constexpr double Quadric::operator()(const double& x) const
{
  return c[0] + x * (c[1] + x * c[2]);
}



















