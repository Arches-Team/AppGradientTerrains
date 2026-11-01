// Curve
#pragma once

#include "vectorset.h"

class QuadricCurve2Set;
class CubicCurve2Set;




class PointCurve2 :public VectorSet2 {
protected:
  bool closed = false; //!< Closed curve flag.
public:
          explicit PointCurve2(const QVector<Vector2>&, bool = false);
  
  //! Empty.
  ~PointCurve2() {}

  int Size() const;
    
  
    
    
        
        
  // Projection
  double R(const Vector2&, double&, int&) const;
  double R(const Vector2&, double&, int&, int&) const;

  };



/*!
\brief Return the size of the sampled curve.
*/
inline int PointCurve2::Size() const
{
  return v.size();
}
