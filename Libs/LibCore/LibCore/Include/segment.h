// Segment
#pragma once

// Forward declare seldom used pre-processing intensive Qt class

#include "box.h"

// Forward declare classes: Quadric
class Quadric;

// Segment class
class Segment
{
protected:
  Vector a = Vector::Null, b = Vector::Z;   //!< End vertices of the segment.
public:
  //! Empty.
  Segment() {}
  explicit Segment(const Vector&, const Vector&);
  explicit Segment(const Segment2&, const double&, const double&);

  //! Empty.
  ~Segment() {}

      
          
  
  // End vertices
  Vector Vertex(int) const;
  Vector& Vertex(int);

    
  
        friend std::ostream& operator<<(std::ostream&, const Segment&);

        
      };

/*!
\brief Creates a segment given its end vertices.
\param a, b End vertices of the segment.
*/
inline Segment::Segment(const Vector& a, const Vector& b) :a(a), b(b)
{
}

//! Return one of the end vertex of the axis.
inline Vector Segment::Vertex(int i) const
{
  if (i == 0) return a;
  else return b;
}

//! Return one of the end vertex of the axis.
inline Vector& Segment::Vertex(int i)
{
  if (i == 0) return a;
  else return b;
}





// Segment class
class Segment2
{
protected:
  Vector2 a = Vector2::Null, b = Vector2::X; //!< End vertices of the segment.
public:
    explicit Segment2(const Vector2&, const Vector2&);
    
  // End vertices
  Vector2 Vertex(int) const;
  Vector2& Vertex(int);

    
    
  
      
    
    friend std::ostream& operator<<(std::ostream&, const Segment2&);

  friend bool operator==(const Segment2&, const Segment2&);

    
    double R(const Vector2&, double&) const;
  
    
    static constexpr const double epsilon = 1e-8; };

/*!
\brief Creates a planar segment given end vertices.
\param a, b End vertices of the segment.
*/
inline Segment2::Segment2(const Vector2& a, const Vector2& b) :a(a), b(b)
{
}


//! Return one of the end vertex of the axis.
inline Vector2 Segment2::Vertex(int i) const
{
  if (i == 0) return a;
  else return b;
}

//! Return one of the end vertex of the axis.
inline Vector2& Segment2::Vertex(int i)
{
  if (i == 0) return a;
  else return b;
}










class Circle2;




class Polygon2;
class Polygons2;






