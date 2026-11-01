// Matrix 

#include "matrix.h"

/*!
\class Matrix2 matrix.h
\brief This class implements \f$ 2^2 \f$  matrix.

Operators have been overloaded to behave as expected. Thus it is possible to write:
\code
Matrix2 a = Matrix2(Vector2(1.0, 2.0));  // Scale
Matrix2 b = Matrix2::Rotation(Math::Pi / 4.0); // Rotation matrix
Vector2 u(2.0, -1.0); // Vector
Vector2 v = (a*b)*u;  // Matrix vector product
\endcode

\ingroup MathGroup
*/

const double Matrix2::epsilon = 1.0e-8;

const Matrix2 Matrix2::RotationHalfPi(0.0, 1.0, -1.0, 0.0);
const Matrix2 Matrix2::RotationOneThirdPi(0.5, Math::Sqrt3 / 2.0, -Math::Sqrt3 / 2.0, 0.5);

/*!
\brief This static member defines the null matrix.
*/
const Matrix2 Matrix2::Null(0.0);

/*!
\brief This static member defines the identity matrix.
*/
const Matrix2 Matrix2::Identity(1.0);/*!
\brief Create a rotation matrix.

\param a Angle in radian.
*/
Matrix2 Matrix2::Rotation(const double& a)
{
  double c = cos(a);
  double s = sin(a);
  return Matrix2(c, s, -s, c);
}/*!
\brief Create an anisotropic tensor matrix.

\param angle The angle of the tensor.
*/
Matrix2 Matrix2::Tensor2(const double& angle)
{
  double c = cos(2.0 * angle);
  double s = sin(2.0 * angle);
  return Matrix2(c, s, s, -c);
}
