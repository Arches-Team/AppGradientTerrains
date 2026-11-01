// Quintics

#pragma once

#include "quartic.h"

class Quintic {
protected:
  double c[6] = { 0.0,0.0,0.0,0.0,0.0,0.0 }; //!< %Array of coefficients.
public:
  //! Empty.
  Quintic() {}
  explicit Quintic(const double&, const double&, const double&, const double&, const double&, const double&);
  Quintic(const Quadric&);
  Quintic(const Cubic&);
  Quintic(const Quartic&);
  //! Empty.
  ~Quintic() {}

  constexpr double& operator[] (int);
  constexpr double operator[] (int) const;

  int CheckDegree() const;

  // Unary operators
  Quintic operator+ () const { return *this; }
  Quintic operator- () const;

  Quintic& operator+= (const Quintic&);
  Quintic& operator-= (const Quintic&);
  Quintic& operator*= (const double&);
  Quintic& operator/= (const double&);

  // Binary operators
  friend Quintic operator+ (const Quintic&, const Quintic&);
  friend Quintic operator- (const Quintic&, const Quintic&);

  friend Quintic operator* (const Quintic&, const double&);
  friend Quintic operator* (const double&, const Quintic&);
  friend Quintic operator/ (const Quintic&, const double&);

  // Derivatives
  Quartic Prime() const;
  Cubic Second() const;
  Quadric Third() const;

  // Lipschitz
  double K(const double&, const double&) const;

  // Evaluates polynomial
  constexpr double operator()(const double&) const;
  constexpr double Derivative(const double&) const;

  // Hermite quintic
  static Quintic Hermite(const double&, const double&, const double&, const double&, const double&, const double&);
  static Quintic Bezier(const double&, const double&, const double&, const double&, const double&, const double&);

  // Solve
  int Solve(double*) const;
  int Solve(double*, const double&, const double&) const;

  void Range(double&, double&, const double& = 0.0, const double& = 1.0) const;

  static double Smooth(const double&);
  static double SmoothCompact(const double&, const double&);
  static double SmoothStep(const double&, const double&, const double&);
  static double Gaussian(const double&, const double&, const double&);
  static double GaussianThick(const double&, const double&, const double&, const double&);

  static Quintic FromRoots(const QVector<double>&);

  friend std::ostream& operator<<(std::ostream&, const Quintic&);
public:
  static const double epsilon; //!< \f$ \varepsilon \f$ value used to check discriminant terms in the root finding process.
};





//! Access to the coefficients of the quintic.
inline constexpr double& Quintic::operator[] (int i)
{
  return c[i];
}

//! Overloaded.
inline constexpr double Quintic::operator[] (int i) const
{
  return c[i];
}

/*!
\brief Evaluates the quintic.
\param x Argument value of the function.
*/
inline constexpr double Quintic::operator()(const double& x) const
{
  return c[0] + x * (c[1] + x * (c[2] + x * (c[3] + x * (c[4] + x * c[5]))));
}











/*!
\brief Compute the value of smooth \f$ C^2 \f$ interpolating function over unit interval.

The quintic is defined as \f$ x^3 (6 x^2 - 15 x + 10) \f$.
Its first and second derivatives at 0.0 and 1.0 are 0.0.

The Lipschitz constant of the smooth quintic over [0,1] is \f$ \lambda = 15/8 \f$.

\param x Argument in [0,1].
\sa Septic::Smooth(), Cubic::Smooth()
*/
inline double Quintic::Smooth(const double& x)
{
  return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
}




