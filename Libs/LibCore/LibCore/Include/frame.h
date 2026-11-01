// Frames 

#pragma once

#include "matrix.h"
#include "ray.h"











// Frame class
class Frame
{
protected:
  Matrix r = Matrix::Identity;  //!< Rotation matrix.
  Vector t = Vector::Null;  //!< Translation vector.
public:
  Frame(const Matrix & = Matrix::Identity, const Vector & = Vector::Null);
  
  
  //! Empty
  ~Frame() {}

  
          
    
    
  
  Vector Transform(const Vector&) const;
    Vector TransformDirection(const Vector&) const;
  
    
  friend std::ostream& operator<<(std::ostream&, const Frame&);

        
public:
  static const Frame Id; //!< Identity.
};



/*!
\brief Transform a point out of the frame coordinate system.
\param p Point.
*/
inline Vector Frame::Transform(const Vector& p) const
{
  return r * p + t;
}


/*!
\brief Transform a direction vector out of the frame coordinate system.
\param n %Vector.
*/
inline Vector Frame::TransformDirection(const Vector& n) const
{
  return r * n;
}


// Frame class for instances
class FrameScaled :protected Frame
{
protected:
  Vector s; //!< Scaling vector
public:
  explicit FrameScaled(const Matrix & = Matrix::Identity, const Vector & = Vector::Null, const Vector & = Vector(1.0));
  explicit FrameScaled(const Vector&, const Vector&);
  
    
  //! Empty
  ~FrameScaled() {}

  // Access to rotation matrix, translation vector, scaling vector
  Matrix R() const;
  Vector T() const;
  Vector S() const;

          
  
  // Translations 
  static FrameScaled Translation(const Vector&);

      
    
  
  Vector Transform(const Vector&) const;
    Vector TransformDirection(const Vector&) const;
  
    
  
  friend std::ostream& operator<<(std::ostream&, const FrameScaled&);
public:
  static const FrameScaled Id; //!< Identity.
};

/*!
\brief Returns the rotation matrix of the frame.
*/
inline Matrix FrameScaled::R() const
{
  return r;
}

/*!
\brief Returns the translation vector of the frame.
*/
inline Vector FrameScaled::T() const
{
  return t;
}
