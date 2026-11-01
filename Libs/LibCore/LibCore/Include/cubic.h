// Cubics

#pragma once

#include "quadric.h"

class Cubic {
protected:
  double c[4] = { 0.0,0.0,0.0,0.0 }; //!< %Array of coefficients.
public:
  //! Empty.
  Cubic() {}
  explicit Cubic(const double&, const double&, const double&, const double&);
  Cubic(const Quadric&);
  //! Empty.
  ~Cubic() {}

  constexpr double& operator[] (int);
  constexpr double operator[] (int) const;

  int CheckDegree() const;

  // Unary operators

  // Overloaded out of consistency.
  Cubic operator+ () const { return *this; }
  Cubic operator- () const;

  Cubic& operator+= (const Cubic&);
  Cubic& operator-= (const Cubic&);
  Cubic& operator*= (const double&);
  Cubic& operator/= (const double&);

  // Binary operators
  friend Cubic operator+ (const Cubic&, const Cubic&);
  friend Cubic operator- (const Cubic&, const Cubic&);

  friend Cubic operator* (const Cubic&, const double&);
  friend Cubic operator* (const double&, const Cubic&);
  friend Cubic operator/ (const Cubic&, const double&);
  friend Cubic operator* (const Quadric&, const Linear&);
  Cubic operator+(const double&) const;

  // Evaluates cubic
  constexpr double operator()(const double&) const;
  constexpr double Derivative(const double&) const;

  // Derivatives
  Quadric Prime() const;
  Linear Second() const;

  static Cubic Compose(const Cubic&, const Linear&);

  // Solve
  int Solve(double*) const;
  int Solve(double*, const double&, const double&) const;
  int SolveNormalized(double*) const;

  void Range(double&, double&, const double& = 0.0, const double& = 1.0) const;

  double K(const double&, const double&) const;

  friend std::ostream& operator<<(std::ostream&, const Cubic&);

  static Cubic Hermite(const double&, const double&, const double&, const double&);
  static Cubic Bezier(const double&, const double&, const double&, const double&);
  static Cubic Spline(const double&, const double&, const double&, const double&);
  static double Interpolation(const double&, const double&, const double&, const double&, const double&);

  static double Smooth(const double&, const double&);
  static double SmoothCompact(const double&, const double&);
  static double SmoothCompact(const double&, const double&, const double&);
  static double Smooth(const double&);
  static double SmoothStep(const double&, const double&, const double&);

  static double Gaussian(const double&, const double&, const double&);
  static double GaussianThick(const double&, const double&, const double&, const double&);
  static double Sigmoid(const double&, const double&, const double&);
  static double Warp(const double&);

  // Bernstein
  static Cubic Bernstein(int);
  static double Bernstein(int, const double&);
public:
  static const double epsilon; //!< \f$ \varepsilon \f$ value used to check if the cubic term is so close to 0 that the cubic is in fact a quadric.
};

/*!
\brief Creates a cubic.

Coefficients start from the highest degree to the lowest.
\param a, b, c, d Coefficients of the cubic \f$ a x^3 + b x^2 + c x + d \f$.
*/
inline Cubic::Cubic(const double& a, const double& b, const double& c, const double& d) 
{
  Cubic::c[3] = a;
  Cubic::c[2] = b;
  Cubic::c[1] = c;
  Cubic::c[0] = d;
}

//! Creates a cubic given a quadric.
inline Cubic::Cubic(const Quadric& p)
{
  c[3] = 0.0;
  c[2] = p[2];
  c[1] = p[1];
  c[0] = p[0];
}

//! Access class components.
inline constexpr double& Cubic::operator[] (int i)
{
  return c[i];
}

//! Overloaded.
inline constexpr double Cubic::operator[] (int i) const
{
  return c[i];
}

/*!
\brief Evaluates the cubic.

\param x Value.
*/
inline constexpr double Cubic::operator()(const double& x) const
{
  return c[0] + x * (c[1] + x * (c[2] + x * c[3]));
}
















/*!
\brief Compute the value of a \f$ C^2 \f$ smooth interpolating function \f$ (1-x/r)^3 \f$.

It is possible to implement Wyvill's (Galin's in fact) smoothing kernel by passing the squared
distance to the skeleton and the squared radius to this function. For example,
given a box skeleton:
\code
Vector p(-2.0,3.0,5.0);
double d=Box(1.0).R(p); // Squared distance to a box
double r=4.0; // Radius;
d=d>r*r?0.0:Cubic::Smooth(d,r*r); // Direct implementation of Wyvill's smoothing kernel
\endcode

\sa Smooth(const double&, const double&, const double&)
\sa SmoothCompact(const double&, const double&)

\param x Squared distance.
\param r Squared radius.
*/
inline double Cubic::Smooth(const double& x, const double& r)
{
  return Math::Cube(1.0 - x / r);
}



/*!
\brief Compute the value of a \f$ C^1 \f$ smooth interpolating function.

The cubic is defined as \f$ x^2 (3 - 2 x) \f$. Its first derivatives at 0.0 and 1.0 are 0.0.

The Lipschitz constant of the smooth cubic over [0,1] is \f$ \lambda = 3/2 \f$.

\param x Argument in [0,1].

\sa Quintic::Smooth(), Septic::Smooth()
*/
inline double Cubic::Smooth(const double& x)
{
  return x * x * (3.0 - 2.0 * x);
}

/*!
\brief Compute a smooth cubic step.

The code is slightly more efficient than:
\code
double y=Cubic::Smooth(Linear::Step(x,a,b));
\endcode
\param a, b Interval values.
\param x Input value.
\sa Quintic::SmoothStep(), Septic::SmoothStep()
*/
inline double Cubic::SmoothStep(const double& x, const double& a, const double& b)
{
  if (x < a)
  {
    return 0.0;
  }
  else if (x > b)
  {
    return 1.0;
  }
  else
  {
    return Cubic::Smooth((x - a) / (b - a));
  }
}




