// Heightfield

#include "gputerrainsimu.h"
#include "heightfieldshader.h"

/*!
\class HeightFieldShader heightfieldshader.h
\brief Relief shading class.

The texture image representing the shaded terrain for Hugo's 2024 paper:
 \code
 return LaplacianShading(Palette::HugoShading, 1.6, 0.6);
\endcode

<h3>Lifetime of the heightfield</h3>

A shader built from a named heightfield only stores a reference to it: the heightfield must outlive the shader,
and modifications of the heightfield are reflected in the subsequent images.
A shader built from a temporary heightfield stores a copy of it, so that the following code is safe:
\code
HeightFieldShader shader(hf.Resized(512, 512));
QImage image = shader.Relief();
\endcode
Shaders cannot be copied nor assigned.

<h3>GPU methods</h3>

LaplacianShading(), ShadedRelief(), ShadedReliefFractionnalGradient(), CartographicGreen() and CartographicGreenYellow()
perform analyses with GPUHeightFieldAnalysis, and therefore require a current OpenGL context supporting compute shaders.
*/

/*!
\brief Create a shader.
\param field Reference to a heightfield.
*/
HeightFieldShader::HeightFieldShader(const HeightField& field) :heightfield(field)
{}

/*!
\brief Create a shader from a temporary heightfield.

The heightfield is stored in the shader, so that the shader may outlive the temporary,
for instance with HeightFieldShader shader(hf.Resized(512, 512)).
\param field Temporary heightfield.
*/
HeightFieldShader::HeightFieldShader(HeightField&& field) :owned(std::move(field)), heightfield(owned)
{}

/*!
\brief Empty.
*/
HeightFieldShader::~HeightFieldShader()
{
  //std::cout << "HeightFieldShader::~HeightFieldShader" << std::endl;
}/*!
\brief Diffuse lighting with enhanced curvature shaded relief.

Create a texture representing the shaded terrain.
*/
QImage HeightFieldShader::Relief(bool enhanced) const
{
  double ra, rb;

  // Get elevation range
  heightfield.GetRange(ra, rb);

  // Modify range by a few centimeters
  ra -= 0.05;
  rb += 0.05;

  // Shading
  QImage shading = QImage(heightfield.GetQtSize(), QImage::Format_ARGB32);

  // Light
  ScalarField2 light = heightfield.Light(Normalized(Vector(-1.0, 0.5, 2.5)));

  // Laplacian for details
  ScalarField2 laplacian = heightfield.Laplacian();

  laplacian.Step(-0.20, 0.20);

  // Height field
  for (int j = 0; j < heightfield.GetSizeY(); j++)
  {
    QRgb* line = reinterpret_cast<QRgb*>(shading.scanLine(j));
    for (int i = 0; i < heightfield.GetSizeX(); i++)
    {
      // Normalized height
      double z = Linear::Step(heightfield.at(i, j), ra, rb);

      // Height shading: color is a linear interpolation of height colors
      Color cz = Color::Lerp(z, Color(0.75, 0.725, 0.70), Color(0.95, 0.925, 0.90));

      double s = light(i, j);
      s *= s;
      s *= s;

      double la = 0.0;

      if (enhanced == true)
      {
        la = 1.0 - 2.0 * laplacian(i, j);

        la *= fabs(la);
      }
      Color c = cz * ((1.0 + s) / 2.0 + 0.15 * la);

      c = c.Clamp();
      line[i] = c.GetQt().rgb();
    }
  }
  return shading;
}