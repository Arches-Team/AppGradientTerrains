// Curve
#pragma once


#include "box.h"







// Quadric curve class
class QuadricCurve2
{
protected:
  Quadric x, y; //!< Quadric functions for every coordinate.
public:
  //! Empty.
  QuadricCurve2() {}
  explicit QuadricCurve2(const Quadric&, const Quadric&);
  
  
    
    
  // Evaluates curve
  Vector2 operator()(const double&) const;
  Vector2 Eval(const double&) const;

  
  // Computes the tangent curve equation
  Vector2 Tangent(const double&) const;
  
  // Get bounding box
  Box2 GetBox() const;
  
  // Compute distance between point and curve
  double R(const Vector2&, double&) const;

  // Compute the curvilign abscissa of a point on the curve
  double S(const double&, int = 256) const;
  double U(const double&, int = 256) const;

      
    
      
    static QuadricCurve2 Bezier(const Vector2&, const Vector2&, const Vector2&);
    friend std::ostream& operator<<(std::ostream&, const QuadricCurve2&);

  public:
  protected:
  };




















