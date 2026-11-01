// Quartic polynomials

#pragma once

#include "cubic.h"

class Quartic {
protected:
  double c[5] = { 0.0,0.0,0.0,0.0,0.0 }; //!< %Array of coefficients.
public:
  Quartic() {}       //! Empty.

  constexpr double& operator[] (int);
  constexpr double operator[] (int) const;

  
    
        
  // Binary operators
  friend Quartic operator+ (const Quartic&, const Quartic&);
  friend Quartic operator- (const Quartic&, const Quartic&);

  friend Quartic operator* (const Quartic&, const double&);
  friend Quartic operator* (const double&, const Quartic&);
  friend Quartic operator/ (const Quartic&, const double&);

  friend Quartic operator* (const Quadric&, const Quadric&);

      
  
  
  // Evaluates polynomial
  constexpr double operator()(const double&) const;
  
    
  
  friend std::ostream& operator<<(std::ostream&, const Quartic&);
protected:
  public:
  static const double epsilon; //!< Epsilon value used to check discriminant terms in the root finding process.
private:
  static double limit; //!< Epsilon value used to limit coefficients.
};




//! Access to the coefficients of the quartic.
inline constexpr double& Quartic::operator[] (int i)
{
  return c[i];
}

//! Overloaded.
inline constexpr double Quartic::operator[] (int i) const
{
  return c[i];
}

/*!
\brief Evaluates the quartic.
\param x Argument value of the function.
*/
inline constexpr double Quartic::operator()(const double& x) const
{
  return c[0] + x * (c[1] + x * (c[2] + x * (c[3] + x * c[4])));
}










