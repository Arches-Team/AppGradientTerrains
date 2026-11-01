// Circle

#pragma once

#include <QtCore/QVector> 

#include "box.h"

// Circle class
class Circle
{
protected:
  Vector c = Vector::Null;    //!< Center of the circle.
  Vector axis = Vector::Z; //!< %Axis.
  double r = 1.0;    //!< Radius.
public:
        
  
  
      
    
      
  
      // Stream
  friend std::ostream& operator<<(std::ostream&, const Circle&);

      protected:
  static constexpr const double epsilon = 1.0e-10; //!< Internal epsilon constant.
};







class QuadricCurve2;
class CubicCurve2;

class Circle2
{
protected:
  Vector2 c = Vector2::Null; //!< Center.
  double r = 1.0; //!< Radius.
public:
    explicit Circle2(const double&);
  explicit Circle2(const Vector2&, const double&);
      
      
      Vector2 Vertex(const double&) const;
  
  
    
            
    
  Box2 GetBox() const;

        
      
      
      
  
  // Stream
  friend std::ostream& operator<<(std::ostream&, const Circle2&);

      
    
  
  public:
  static const Circle2 Unit; //!< Unit circle.
  static const Circle2 Infinite; //!< Infinite circle.
protected:
  static constexpr const double Epsilon = 1.e-6; //!< %Epslion for intersection tests
};





/*!
\brief Compute the axis aligned bounding box of a circle.
*/
inline Box2 Circle2::GetBox() const
{
  return Box2(c, r);
}











