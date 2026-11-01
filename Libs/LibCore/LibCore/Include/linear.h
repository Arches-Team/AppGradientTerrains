// Linear polynomials

#pragma once

#include "mathematics.h"

class Linear {
protected:
  double c[2] = { 0.0 }; //!< %Array of coefficients.
public:
  //! Empty.
  Linear() {}
      
  constexpr double& operator[] (int);
  constexpr double operator[] (int) const;

    
        
  // Binary operators
  friend Linear operator+ (const Linear&, const Linear&);
  friend Linear operator- (const Linear&, const Linear&);

  friend Linear operator* (const Linear&, const double&);
  friend Linear operator* (const double&, const Linear&);
  friend Linear operator/ (const Linear&, const double&);

  // Evaluates polynomial
  constexpr double operator()(const double&) const;

      
  static double Step(const double&, const double&, const double&);
        
  friend std::ostream& operator<<(std::ostream&, const Linear&);
public:
  static const Linear Id; // Identity.
};



//! Access class components.
inline constexpr double& Linear::operator[] (int i)
{
  return c[i];
}

//! Overloaded.
inline constexpr double Linear::operator[] (int i) const
{
  return c[i];
}

/*!
\brief Evaluates the linear function.
\param x Argument.
*/
inline constexpr double Linear::operator()(const double& x) const
{
  return c[0] + x * c[1];
}












/*!
\brief Create a linear step.
\param x Value
\param a, b Interval values.
\return Real in unit inverval.
\sa Cubic::SmoothStep, Quintic::SmoothStep
*/
inline double Linear::Step(const double& x, const double& a, const double& b)
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
    return (x - a) / (b - a);
  }
}



