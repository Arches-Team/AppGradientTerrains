// Heightfield

#include "heightfield.h"


/*!
\brief Compute the Lipschitz constant of the signed distance field defined as f(x,y,z)=z-h(x,y).
*/
double HeightField::K() const
{
  // Lipschitz
  double lipschitz = ScalarField2::K();

  return sqrt(1.0 + lipschitz * lipschitz);
}/*!
\brief Compute the direct lighting.

\param u Light direction.
\param cosine Boolean, set to true to use scalar product between normal and light, false to use (1+c)/2.
*/
ScalarField2 HeightField::Light(const Vector& u, bool cosine) const
{
  ScalarField2 sf(Box2(a, b), nx, ny, 1.0);

  if (cosine == true)
  {
    for (int i = 0; i < nx; i++)
    {
      for (int j = 0; j < ny; j++)
      {
        double light = Normal(i, j) * u;

        light = Math::Max(0.0, light);


        sf(i, j) = light;
      }
    }
  }
  else
  {
    for (int i = 0; i < nx; i++)
    {
      for (int j = 0; j < ny; j++)
      {
        double light = Normal(i, j) * u;

        light = 0.5 * (1.0 + light);

        sf(i, j) = light;
      }
    }

  }

  return sf;
}