// Vector  

// Self include
#include "evector.h"

/*!
\class Vector2 evector.h
\brief Vectors in two dimensions.

This class implements most operators and member functions as for Vector class.

\ingroup MathGroup
*/

const Vector2 Vector2::Null = Vector2(0.0, 0.0);

const Vector2 Vector2::X = Vector2(1.0, 0.0);
const Vector2 Vector2::Y = Vector2(0.0, 1.0);/*!
\brief Compute the angle of a vector.

The vector need not be normalized.
*/
double Vector2::Angle() const
{
  return atan2(c[1], c[0]);
}

/*!
\brief Compute the signed angle from this vector to the argument vector, in radians.

The vectors need not be normalized. The result is in \f$ (-\pi, \pi] \f$.

\sa Vector2::Angle() const

\param b %Argument vector.
*/
double Vector2::Angle(const Vector2& b) const
{
  Vector2 ba(b * (*this), b * Orthogonal());
  return ba.Angle();
}
#include <QtCore/QStringList>
