#include "turbulence.h"

/*!
\class NoiseTurbulence2 turbulence.h
\brief %Turbulence computed from a modified noise function.

A turbulence field is characterized by the number of
octaves, the attenuation of the amplitude of the noise summed at every
step, and by the amplification of the wavelength.
*/
const Matrix2 NoiseTurbulence2::R2 = Matrix2::Rotation(4.0 * Math::Pi / 19.0);


/*!
\brief Creates a turbulence field.
\param v, a, l Base value, amplitude and wavelength.
\param alpha, lambda Amplitude and wavelength attenuation (should be less than 1.0 in general).
\param octaves Number of octaves.
\param t Offset vector, in the units of the points: it is subtracted from the point before it is scaled by the wavelength.
The turbulence is planar, only the two first coordinates are used.
*/
NoiseTurbulence2::NoiseTurbulence2(const double& v, const double& a, const double& l, const double& alpha, const double& lambda, int octaves, const Vector& t) :v0(v), a0(a), l0(l), alpha(alpha), lambda(lambda), octaves(octaves), t(t)
{}

/*!
\brief Destroys a turbulence instance.
*/
NoiseTurbulence2::~NoiseTurbulence2()
{}


/*!
\brief Compute the turbulence value at a given point.

Sums a series of scaled noise functions.
\param p Point.
*/
double NoiseTurbulence2::Value(const Vector2& p) const
{
  Vector2 q = p - Vector2(t);
  // First wave
  double tu = v0;
  double l = l0;
  double a = a0;

  // Next waves
  for (int i = 0; i < octaves; i++)
  {
    tu += a * Noise2::Value(q / l);
    l *= lambda;
    a *= alpha;
    q = R2 * q;
  }

  return tu;
}
