// Rays

#pragma once

#include "evector.h"

// Forward declare Matrix class for ray rotation
class Matrix;
class Frame;
class Frame2;

// Ray class
class Ray
{
protected:
  Vector c = Vector::Null; //!< Origin of the ray.
  Vector n = Vector::Z; //!< Direction.
public:
  //! Empty.
  Ray() {}
  explicit Ray(const Vector&, const Vector&);
  
  
  Vector operator()(const double&) const;

    
        
  // Functions to access Vector class components
  Vector Origin() const;
  Vector Direction() const;

  friend std::ostream& operator<<(std::ostream&, const Ray&);
};

/*!
\brief Creates a ray.

The direction should be unit:
\code
Ray ray(Vector(0.0,0.0,0.0),Normalized(Vector(2.0,-1.0,3.0)));
\endcode
\param p Origin.
\param d Direction (should be unit vector).
*/
inline Ray::Ray(const Vector& p, const Vector& d)
{
  c = p;
  n = d;
}

/*!
\brief Return the origin of the ray.
*/
inline Vector Ray::Origin() const
{
  return c;
}

/*!
\brief Return the direction of the ray.
*/
inline Vector Ray::Direction() const
{
  return n;
}

/*!
\brief Computes the location of a vertex along the ray.
\param t Parameter.
*/
inline Vector Ray::operator()(const double& t) const
{
  return c + t * n;
}








