// Plane

#pragma once

#include "matrix.h"
#include "ray.h"

// Forward declare 
class Line;
class Segment;
class Quadric;
class Frame;
class Box;
class Sphere;
class Axis;

// Plane 
class Plane {
protected:
  Vector n = Vector::Z;  //!< %Plane normal.
  double cqc = 0.0;  //!< %Plane coefficient.
public:
    explicit Plane(const Vector&, const double&);
  explicit Plane(const Vector&, const Vector&);
  
  //! Empty.
  ~Plane() {}
  bool Intersect(const Ray&, double&) const;
  
        
    
  
        
        
    
    
  
  
  
  friend std::ostream& operator<<(std::ostream&, const Plane&);

      
    public:
  static const double epsilon; //!< Small value for checking intersection with a ray.
  static const Plane XY;   //!< Reference z-axis horizontal plane.
  static const Plane YZ;   //!< Reference x-axis plane.
  static const Plane ZX;   //!< Reference y-axis plane.
};

/*!
\brief Creates a plane given normal and distance.

The equation of the plane is: \f$\ep \cdot \en - c = 0\f$.

\param n %Normal, should be unit.
\param c %Plane constant.
*/
inline Plane::Plane(const Vector& n, const double& c) :n(n), cqc(c)
{
}





