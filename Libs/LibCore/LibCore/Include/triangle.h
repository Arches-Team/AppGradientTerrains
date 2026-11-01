// Triangle

#pragma once

#include <QtCore/QVector>

#include "sphere.h"

// Forward declare classes: Plane, Segment
class Plane;

class Segment;
class Segment2;

// Triangle
class Triangle
{
protected:
  Vector p[3]; //!< %Array of vertices.
public:
    explicit Triangle(const Vector&, const Vector&, const Vector&);

  
    
  
      
            
    Vector AreaNormal() const;
  
    
    
  
      
    
    
  // Stream
  friend std::ostream& operator<<(std::ostream&, const Triangle&);

  
      
  protected:
  static constexpr const double epsilon = 1.0e-7; //!< Internal epsilon constant.
protected:
  };





/*!
\brief Create a triangle.
\param a,b,c Vertices of the triangle.
*/
inline Triangle::Triangle(const Vector& a, const Vector& b, const Vector& c) :p{ a,b,c }
{
}

// Forward declare classes: Circle2
class Circle2;






