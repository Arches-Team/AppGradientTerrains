// Box
#pragma once

// Forward declare seldom used pre-processing intensive Qt class

#include <QtGui/QPen>
#include <QtGui/QBrush>
#include <QtCore/QRect>
#include <QtCore/QSize>

#include "frame.h"

class Segment;
class Plane;
class Axis;
class Ia;

class Box
{
protected:
  Vector a = Vector(0.0), b = Vector(1.0); //!< Lower and upper vertexex.
public:
  //! Empty.
  Box() {}
  explicit Box(const double&);
  explicit Box(const Vector&);
  explicit Box(const Vector&, const Vector&);
                  
  //! Empty.
  ~Box() {}

  // Access vertexes
  Vector& operator[] (int);
  Vector operator[] (int) const;

  // Comparison
  friend bool operator==(const Box&, const Box&);
  friend bool operator!=(const Box&, const Box&);

  // Acces to vertexes
  Vector Center() const;
  Vector Vertex(int) const;
    
      
      
    
    int Intersect(const Ray&, double&, double&) const;
  
      
  
    
    
      
    
      void Extend(const Vector&);
  
    
  
  
  // Minkowski sum
  friend Box operator+(const Box&, const Box&);
  friend Box operator*(const double&, const Box&);
  friend Box operator*(const Box&, const double&);

            
    
  friend std::ostream& operator<<(std::ostream&, const Box&);

  
    
  public:
  static constexpr const double epsilon = 1.0e-5; //!< Epsilon value used to check intersections and some round off errors and for ray intersection tests.
  static const Box Infinity; //!< Largest box.
  static const Box Null; //!< Null box, equivalent to: \code Box(Vector(0.0)); \endcode 
  static const Box Unit; //!< Unit box.
  static const int edge[24]; //!< Edge vertex indexes.
  static const Vector normal[6]; //!< Face normals.
};

//! Returns either end vertex of the box.
inline Vector& Box::operator[] (int i)
{
  if (i == 0) return a;
  else return b;
}

//! Overloaded.
inline Vector Box::operator[] (int i) const
{
  if (i == 0) return a;
  else return b;
}




//! Returns the center of the box.
inline Vector Box::Center() const
{
  return 0.5 * (a + b);
}




/*!
\brief Returns the k-th vertex of the box.

\image html box-indexes.png

The returned vector is computed by analysing the first three bits of k as follows:
\code
Vector vertex=Vector((k&1)?b[0]:a[0],(k&2)?b[1]:a[1],(k&4)?b[2]:a[2]);
\endcode
*/
inline Vector Box::Vertex(int k) const
{
  return Vector((k & 1) ? b[0] : a[0], (k & 2) ? b[1] : a[1], (k & 4) ? b[2] : a[2]);
}










class Line2;
class Segment2;

class Box2
{
protected:
  Vector2 a = Vector2(0.0), b = Vector2(1.0); //!< Lower and upper vertexes of the box.
public:
  //! Empty
  Box2() {}
  explicit Box2(const double&);
  explicit Box2(const double&, const double&);
    explicit Box2(const Vector2&, const Vector2&);
  explicit Box2(const Vector2&, const double&);
    explicit Box2(const Box&);
            
  Vector2& operator[] (int);
  Vector2 operator[] (int) const;

  Vector2 Size() const;
      Vector2 Diagonal() const;
  
    
  // Access to vertexes
  Vector2 Center() const;
      
  
  
      
      
            
    
  
    
      Box2 Extended(const double&) const;

    
  // Inside
  bool Inside(const Vector2&) const;
  
    
  
  Box ToBox(const double&, const double&) const;

  // Comparison
  friend bool operator==(const Box2&, const Box2&);
  friend bool operator!=(const Box2&, const Box2&);

  
  // Minkowski sum
  friend Box2 operator+(const Box2&, const Box2&);
  friend Box2 operator*(const double&, const Box2&);
  friend Box2 operator*(const Box2&, const double&);

    
        
  friend std::ostream& operator<<(std::ostream&, const Box2&);
  
    
  QRectF GetQtRect() const;

      
  Segment2 GetSegment(const double&, bool = true) const;

  
        
  public:
  static const double epsilon;
  static const Box2 Infinity;
  static const Box2 Null;
  static const Box2 Unit;
protected:
  static const int edge[8]; //!< Edge vertex indexes.
};

/*!
\brief Create a box.

Note that is possible to create a box using Vector as parameters
as the compiler will call the constructor Vector2::Vector2(const Vector&).

\param a,b Points.
*/
inline Box2::Box2(const Vector2& a, const Vector2& b) :a(a), b(b)
{}

/*!
\brief Create a box in the plane given a box.
\param box The box.
*/
inline Box2::Box2(const Box& box) :a(box[0]), b(box[1])
{}

/*!
\brief Creates a box.

\param c Center.
\param r Radius.
*/
inline Box2::Box2(const Vector2& c, const double& r)
{
  a = c - Vector2(r);
  b = c + Vector2(r);
}


/*!
\brief Create a square box centered at the origin and of given half side length.

This is equivalent to:
\code
Box2 box(Vector2(0.0),2.0);  // Simplified constructor Box2(2.0);
\endcode
\param r Half side length.
*/
inline Box2::Box2(const double& r)
{
  a = -Vector2(r);
  b = Vector2(r);
}
/*!
\brief Create a box centered at the origin and of given dimensions.

This is equivalent to:
\code
double width, height;
Box2 box(Vector2(-width/2.0,-height/2.0),Vector2(width/2.0,height/2.0);  // Simplified constructor Box2(width,height);
\endcode
\param x,y %Size.
*/
inline Box2::Box2(const double& x, const double& y)
{
  a = -Vector2(x / 2.0, y / 2.0);
  b = Vector2(x / 2.0, y / 2.0);
}



//! Returns either end vertex of the box.
inline Vector2& Box2::operator[] (int i)
{
  if (i == 0) return a;
  else return b;
}

//! Overloaded.
inline Vector2 Box2::operator[] (int i) const
{
  if (i == 0) return a;
  else return b;
}

/*!
\brief Compute the size (width and height) of a box.
*/
inline Vector2 Box2::Size() const
{
  return b - a;
}








/*!
\brief Returns the diagonal of the box.
*/
inline Vector2 Box2::Diagonal() const
{
  return (b - a);
}

/*!
\brief Returns the center of the box.
*/
inline Vector2 Box2::Center() const
{
  return 0.5 * (a + b);
}









