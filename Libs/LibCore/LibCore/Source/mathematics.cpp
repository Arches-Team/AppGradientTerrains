// Mathematics
#include <ostream>

#include "mathematics.h"
/*!
\brief Bi-cubic interpolation between four values, given partial derivatives.

The values are given in trigonometric order.

\param u,v Interpolation coefficients.
\param a00,a10,a11,a01 Interpolated values.
\param u00,u10,u11,u01,v00,v10,v11,v01 Partial derivatives with respect to u and v.
\param x00,x10,x11,x01 Cross derivatives.
*/
double Math::BiCubic(const double& u, const double& v, const double& a00, const double& a10, const double& a11, const double& a01, const double& u00, const double& u10, const double& u11, const double& u01, const double& v00, const double& v10, const double& v11, const double& v01, const double& x00, const double& x10, const double& x11, const double& x01)
{
  double u2 = u * u;
  double v2 = v * v;

  double hu00 = u2 * (2.0 * u - 3.0) + 1.0;
  double hu01 = u2 * (-2.0 * u + 3.0);
  double hu10 = (u2 - 2.0 * u + 1.0) * u;
  double hu11 = u2 * (u - 1.0);

  double hv00 = v2 * (2.0 * v - 3.0) + 1.0;
  double hv01 = v2 * (-2.0 * v + 3.0);
  double hv10 = (v2 - 2.0 * v + 1.0) * v;
  double hv11 = v2 * (v - 1.0);

  return hu00 * (a00 * hv00 + a01 * hv01 + v00 * hv10 + v01 * hv11) +
    hu01 * (a10 * hv00 + a11 * hv01 + v10 * hv10 + v11 * hv11) +
    hu10 * (u00 * hv00 + u01 * hv01 + x00 * hv10 + x01 * hv11) +
    hu11 * (u10 * hv00 + u11 * hv01 + x10 * hv10 + x11 * hv11);
}