// Analytic fields

#include "scalarfield.h"

/*!
\class AnalyticScalarField2 scalarfield.h
\brief A core analytic two-dimensional scalar field.

\ingroup StructureGroup
*/

const double AnalyticScalarField2::epsilon = 1e-6;

// Second order differences are divided by epsilon^2: rounding errors grow as 4 u |f| / epsilon^2 (u = 1.1e-16 for double),
// whereas truncation errors decrease as epsilon^2 |f''''| / 12. The optimal step is about u^(1/4), i.e., 1e-4, which yields
// a relative error close to 1e-8, whereas the step 1e-6 used for the gradient would yield 1e-3.
const double AnalyticScalarField2::epsilonhessian = 1e-4;

/*!
\brief Compute the value of the field.
\param p Point.
*/
#pragma warning(push)
#pragma warning(disable: 4100)  
double AnalyticScalarField2::Value(const Vector2& p) const
{
  return 0.0;
}
#pragma warning(pop)

/*!
\brief Compute the gradient of the field.
\param p Point.
*/
Vector2 AnalyticScalarField2::Gradient(const Vector2& p) const
{
  double x = Value(Vector2(p[0] + epsilon, p[1])) - Value(Vector2(p[0] - epsilon, p[1]));
  double y = Value(Vector2(p[0], p[1] + epsilon)) - Value(Vector2(p[0], p[1] - epsilon));

  return Vector2(x, y) * (0.5 / epsilon);
}

/*!
\brief Compute the local neighborhood in the one-ring aroung a point.

The step is epsilonhessian, since this neighborhood is used for second order differences.
\param p Point.
*/
Matrix AnalyticScalarField2::Local(const Vector2& p) const
{
  const double e = epsilonhessian;

  // a(1 + di, 1 + dj) = f(x + di * e, y + dj * e), listed in storage order (x varies fastest)
  return Matrix(
    Value(Vector2(p[0] - e, p[1] - e)), Value(Vector2(p[0], p[1] - e)), Value(Vector2(p[0] + e, p[1] - e)),
    Value(Vector2(p[0] - e, p[1])), Value(p), Value(Vector2(p[0] + e, p[1])),
    Value(Vector2(p[0] - e, p[1] + e)), Value(Vector2(p[0], p[1] + e)), Value(Vector2(p[0] + e, p[1] + e)));
}

/*!
\brief Compute the hessian at a given point.

\param p Point.
*/
Matrix2 AnalyticScalarField2::Hessian(const Vector2& p) const
{
  // Compute local neighborhood
  Matrix a = Local(p);

  // Second order finite differences
  const double e2 = epsilonhessian * epsilonhessian;

  return Matrix2(
    (a(2, 1) + a(0, 1) - 2.0 * a(1, 1)) / e2,
    (a(1, 2) + a(1, 0) - 2.0 * a(1, 1)) / e2,
    ((a(2, 2) - a(0, 2)) - (a(2, 0) - a(0, 0))) / (4.0 * e2));
}

/*!
\brief Create a discrete scalar field by sampling a given domain.
\param a Domain.
*/
ScalarField2 AnalyticScalarField2::Sample(const Array2& a) const
{
  ScalarField2 s(a);
  const int nx = a.GetSizeX();
  const int ny = a.GetSizeY();

  // Set the scalar field values by evaluating the function over the vertices of the grid
  for (int j = 0; j < ny; j++)
  {
    for (int i = 0; i < nx; i++)
    {
      s(i, j) = Value(a.ArrayVertex(i, j));
    }
  }

  return s;
}