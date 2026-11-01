// Matrix 

#include "matrix.h"


/*!
\class Matrix matrix.h
\brief This class implements \f$ 3^2 \f$  matrix.

Operators have been overloaded so as to behave as expected
with the Vector class.

\image html matrix3.png

Many functions have been coded inline out of efficiency.

The constructors of this class are used to create matrices
in the general case.
See the static member functions Matrix::Rotation() to create
different kinds of rotation matrices. Canonical symmetry matrices should
be created as follows:
\code
Matrix planar=Matrix(Vector(1.0,-1.0,1.0)); // Symmetry, plane y=0
Matrix origin=Matrix(Vector(-1.0,-1.0,-1.0)); // Symmetry around origin
\endcode
More general symmetries can be created using static member functions:
\code
Matrix planar=Matrix::Symmetry(Vector(2.0,-3.0,1.0)); // Symmetry, plane with normal vector as argument
\endcode

Components are stored in a single dimension array, sorting components
by column.

\ingroup MathGroup
*/

const double Matrix::epsilon = 1.0e-8;

/*!
\brief Null matrix.
*/
const Matrix Matrix::Null(0.0);

/*!
\brief Identity matrix.
*/
const Matrix Matrix::Identity(1.0);

const Matrix Matrix::RotationHalfPiZ(0.0, 1.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 1.0);/*!
\brief Compute the angles of the rotation matrix.
*/
Vector Matrix::GetRotationAngles() const
{
  // Angles
  double x, y, z;

  const Matrix& m = *this;

  if (m(0, 2) < +1.0)
  {
    if (m(0, 2) > -1.0)
    {
      y = asin(m(0, 2));
      x = atan2(-m(1, 2), m(2, 2));
      z = atan2(-m(0, 1), m(0, 0));
    }
    else // r02 = -1
    {
      // Not a unique solution: thetaZ - thetaX = atan2(r10,r11)
      y = -Math::HalfPi;
      x = -atan2(m(1, 0), m(1, 1));
      z = 0.0;
    }
  }
  else // r02 = +1
  {
    // Not a unique solution: thetaZ + thetaX = atan2(r10,r11)
    y = +Math::HalfPi;
    x = atan2(m(1, 0), m(1, 1));
    z = 0.0;
  }

  return Vector(x, y, z);
}

