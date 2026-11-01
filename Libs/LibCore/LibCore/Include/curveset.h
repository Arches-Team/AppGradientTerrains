// Curve

#pragma once

#include "curve.h"
















class QuadricCurve2Set
{
protected:
  QVector<QuadricCurve2> curve; //!< Set of quadric curves.
  QVector<double> qlengths;     //!< Length of every curve (internal optimization)
  double qqlength = 0.0;       //!< Total length (internal optimization)
  QVector<Box2> boxes;          //!< Slightly enlarged bounding box of every curve, lower bound used by UV() to skip distant curves (internal optimization)

  void UpdateBoxes();
public:
    explicit QuadricCurve2Set(const QVector<Vector2>&);
    
  ~QuadricCurve2Set();

    QuadricCurve2 operator()(int) const;

  
  
  // Get length
  double GetLength() const;
  
  
    
    int U(const double&, double&) const;
  double R(const Vector2&, double&, int&) const;
  
  
      
      };


/*!
\brief Read only access to the elements of the piecewise quadric curve.
*/
inline QuadricCurve2 QuadricCurve2Set::operator()(int k) const
{
  return curve[k];
}

/*!
\brief Return the length of the curve.
*/
inline double QuadricCurve2Set::GetLength() const
{
  return qqlength;
}

