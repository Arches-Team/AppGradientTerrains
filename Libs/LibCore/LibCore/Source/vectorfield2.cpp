// Vector fields

#include "vectorfield.h"
#include <curveset.h>

#include <immintrin.h>


/*!
\class VectorField2 vectorfield.h
\brief A base two-dimensional field of Vector2 values.

\ingroup StructureGroup
*/

/*!
\brief Create the field structure.
\param box The box.
\param x,y Size of the array.
\param v Default value of field.
*/
VectorField2::VectorField2(const Box2& box, int x, int y, const Vector2& v) :Array2(box, x, y)
{
  field.fill(v, nx * ny);
}/*!
\brief Multiply the vector field by a scaling factor locally.

At the center, the scaling will be applied and at the boundary it will be equal to 1.
\param center Center.
\param radius Radius.
\param scaling Scaling of the effect.
*/
void VectorField2::Multiply(const Vector2& center, const double& radius, const double& scaling)
{
  // Compute modification Area
  const QRect area = VertexIntegerArea(Circle2(center, radius));

  for (int y = area.y(); y <= area.y() + area.height(); y++)
  {
    for (int x = area.x(); x <= area.x() + area.width(); x++)
    {
      // Distance between central point and current point
      double u = SquaredNorm(center - ArrayVertex(x, y));

      if (u < radius * radius)
      {
        double a = Cubic::Smooth(u, radius * radius);

        // We modify the field proportionnaly to the height difference
        field[VertexIndex(x, y)] *= a * scaling + (1.0 - a); // interpolates scaling and 1
      }
    }
  }
}/*!
\brief Clone part of the vector field from source to destination positions, with a given radius. The destination is blended with a decreasing weight towards the disk boundary where it is null.
\param center Destination position.
\param source Source position.
\param strokebegin Stroke starting position.
\param radius Radius of the disk.
*/
void VectorField2::Clone(const Vector2& center, const Vector2& source, const Vector2& strokebegin, const double& radius)
{
  // Compute modification Area
  const QRect area = VertexIntegerArea(Circle2(center, radius));
  QPoint sourcexy = VertexInteger(source);
  int x0 = sourcexy.x();
  int y0 = sourcexy.y();
  QPoint destxy = VertexInteger(center);

  QPoint beginxy = VertexInteger(strokebegin);
  int xb = beginxy.x();
  int yb = beginxy.y();

  for (int y = area.y(); y <= area.y() + area.height(); y++)
  {
    for (int x = area.x(); x <= area.x() + area.width(); x++)
    {
      // Distance between central point and current point
      double u = SquaredNorm(center - ArrayVertex(x, y));

      if (u < radius * radius)
      {
        double a = Cubic::Smooth(u, radius * radius);
        a = sqrt(a);
        // We modify the field proportionnaly to the height difference
        field[VertexIndex(x, y)] = (1.0 - a) * field[VertexIndex(x, y)] + a * field[VertexIndex(x0 + x - xb, y0 + y - yb)]; // blend
      }
    }
  }
}

/*!
\brief Clone part of the vector field from source to destination positions.

The destination is blended with a decreasing weight towards the disk boundary where it is null.
\param center Target position.
\param lassoedZone Selected area.
\param borderBlendTreshold Border threshold.
*/
void VectorField2::CloneZone(const Vector2& center, const QPolygon& lassoedZone, int borderBlendTreshold)
{
  const QRect area = lassoedZone.boundingRect();
  const QPoint sourceCorner = area.topLeft();
  const QPoint destCorner(center[0] - area.width() / 2, center[1] - area.height() / 2);

  ScalarField2 distField(Array2(Box2(Vector2(0, 0), Vector2(area.width(), area.height())), area.width(), area.height()), 0.0);

  for (int y = 0; y <= area.height(); y++)
  {
    for (int x = 0; x <= area.width(); x++)
    {
      if (lassoedZone.containsPoint(sourceCorner + QPoint(x, y), Qt::OddEvenFill))
      { //the grid contains 0 inside the polygon, and 0 outsides 
        distField(x, y) = 1.0;
      }
    }
  }

  //computes distanceField
  distField = distField.DistanceTransform(0.5);   //the bounds of the polygone is where it goes from 0 to 1, so 0.5 is the limit
  distField.Sqrted();

  distField.SetRange(0.0, 1.0);   //set all the distances to border between 0 and 1
  double blendThreshold = borderBlendTreshold / 100.0; //if the point is less than distToBlend to the border, then it is blended with the original gradient, else it is just cloned as is

  for (int y = 0; y <= area.height(); y++)   //relative position of the point in the bounding rect of the zone to clone
  {
    for (int x = 0; x <= area.width(); x++)
    {
      if (lassoedZone.containsPoint(sourceCorner + QPoint(x, y), Qt::OddEvenFill))
      {  //absolute position of the point to clone
        QPoint sourcePoint = VertexInteger(Vector2(sourceCorner.x() + (double)x, sourceCorner.y() + (double)y));  //the points coordonates in the field object
        QPoint destPoint = VertexInteger(Vector2(destCorner.x() + (double)x, destCorner.y() + (double)y));

        if (InsideVertexIndex(destPoint) && InsideVertexIndex(sourcePoint))
        {
          double blendCoeff = ((distField(x, y) <= blendThreshold) ? (distField(x, y) / blendThreshold) : 1.0);
          //if the point is close enough to the border, we'll blend its gradient with the new gradient's value, else we just take the new gradients value
          blendCoeff = Cubic::Smooth(blendCoeff * blendCoeff);
          blendCoeff = sqrt(blendCoeff);
          field[VertexIndex(destPoint.x(), destPoint.y())] = (1.0 - blendCoeff) * field[VertexIndex(destPoint.x(), destPoint.y())] + (blendCoeff)*field[VertexIndex(sourcePoint.x(), sourcePoint.y())]; //blend
        }
      }
    }
  }
}

/*!
\brief Clone the vectors along a source polyline onto a destination polyline.

Points closer than halfWidth to the destination curve are blended with the rotated vectors taken at the same relative position along the source curve.
\param sourceLine Source polyline.
\param destLine Destination polyline.
\param halfWidth Half width of the cloned band, in vertices.
*/
void VectorField2::CloneBrokenLine(const QVector<Vector2>& sourceLine, const QVector<Vector2>& destLine, int halfWidth)
{
  QuadricCurve2Set sourceCurve(sourceLine);
  QuadricCurve2Set destCurve(destLine);
  double ratio = sourceCurve.GetLength() / destCurve.GetLength(); //the 2 curve probably don't have the same length
  //QRect destRect = destCurve.GetBox().GetQtRect().toRect();
  for (int ab = 0; ab < destCurve.GetLength() * 10; ab++)
  {
    //for each point of the destCurve
    double a = double(ab) / 10.0;
    for (int d = -halfWidth; d < halfWidth; d++)
    {      //for every point on the normal to the curve in a, who is less than halfwidth away from the curve
      double u;   //abscisse curviligne of the sub-curve of the set, where the point of the curveSet with the curviligne abscisse "a" is 

      int i = destCurve.U(a, u);//the index of the curve of the Set where the projection is
      Vector2 onCurvePoint = destCurve(i)(u);
      Vector2 tangente = destCurve(i).Tangent(u);
      Vector2 normale = Normalized(tangente.Orthogonal());
      Vector2 point = onCurvePoint + normale * d;   //point to get the clone gradient value from

      int si; //i but for the source curve
      double su;  //u but for the source curve
      si = sourceCurve.U(a * ratio, su);
      Vector2 onSourceCurvePoint = sourceCurve(si)(su);
      Vector2 sourceTangente = sourceCurve(si).Tangent(su);
      Vector2 sourceNormale = Normalized(sourceTangente.Orthogonal());
      Vector2 pointToClone = onSourceCurvePoint + sourceNormale * d;   //point to get the clone gradient value from

      //convert points into vertex for field
      QPoint sourcePoint = VertexInteger(pointToClone);
      QPoint destPoint = VertexInteger(point);

      if (InsideVertexIndex(destPoint) && InsideVertexIndex(sourcePoint))
      {
        //we need to rotate the gradient we copy by the difference of angles of normale and sourceNormale to match the new orientation
        double angle = sourceTangente.Angle(tangente);
        Matrix2 rotation = Matrix2::Rotation(angle);

        double blendCoeff = (double)(abs(d)) / halfWidth; //blend old and new gradient
        blendCoeff = Cubic::SmoothStep(blendCoeff, 0.4, 1.);
        /*blendCoeff = Cubic::Smooth(blendCoeff * blendCoeff);
        blendCoeff = sqrt(blendCoeff);*/
        field[VertexIndex(destPoint.x(), destPoint.y())] = blendCoeff * field[VertexIndex(destPoint.x(), destPoint.y())] + (1.0 - blendCoeff) * rotation * field[VertexIndex(sourcePoint.x(), sourcePoint.y())];
      }
    }
  }
}

#include "cpu.h"
/*!
\brief Return the field vector at a given array vertex.
\param i,j Integer coordinates of the vertex.
\sa at(int,int)
*/
Vector2 VectorField2::Value(int i, int j) const
{
  return field.at(VertexIndex(i, j));
}

/*!
\brief Compute the divergence field.
\sa Divergence(int, int)
\author Mathieu Gaillard
*/
ScalarField2 VectorField2::Divergence() const
{
  ScalarField2 d(GetBox(), nx, ny);

  for (int i = 0; i < nx; i++)
  {
    for (int j = 0; j < ny; j++)
    {
      d(i, j) = Divergence(i, j);
    }
  }

  return d;
}

/*!
\brief Compute the divergence at a given sample.
\sa Divergence()
\author Mathieu Gaillard
\param i,j Integer coordinates of the sample.
*/
double VectorField2::Divergence(int i, int j) const
{
  double div = 0.0;

  // Derivative along x axis
  if (i == 0)
  {
    div += (at(i + 1, j)[0] - at(i, j)[0]) * inversecelldiagonal[0];
  }
  else if (i == nx - 1)
  {
    div += (at(i, j)[0] - at(i - 1, j)[0]) * inversecelldiagonal[0];
  }
  else
  {
    div += (at(i + 1, j)[0] - at(i - 1, j)[0]) * 0.5 * inversecelldiagonal[0];
  }

  // Derivative along y axis
  if (j == 0)
  {
    div += (at(i, j + 1)[1] - at(i, j)[1]) * inversecelldiagonal[1];
  }
  else if (j == ny - 1)
  {
    div += (at(i, j)[1] - at(i, j - 1)[1]) * inversecelldiagonal[1];
  }
  else
  {
    div += (at(i, j + 1)[1] - at(i, j - 1)[1]) * 0.5 * inversecelldiagonal[1];
  }

  return div;
}/*!
\brief Multiply the vector field by a scalar.
\param s Scalar.
\avx
*/
void VectorField2::Mul(const double& s)
{
#ifdef _MSC_VER
  if (System::Avx())
  {
    double* p = (double*)(field.data());

    const int size = 2 * nx * ny;
    const int offset = size % 4;

    const double ps[4] = { s,s,s,s };
    __m256d savx = _mm256_load_pd(ps);
    for (int i = 0; i < size / 4; i++)
    {
      __m256d avx = _mm256_load_pd(p + i * 4);
      __m256d addavx = _mm256_mul_pd(avx, savx);
      _mm256_store_pd(p + i * 4, addavx);
    }

    for (int i = size - offset; i < size; i++)
    {
      p[i] *= s;
    }
  }
  else
#endif
  {
    const int n = 2 * field.size();
    double* fieldp = (double*)(field.data());

    for (int i = 0; i < n; i++)
    {
      fieldp[i] *= s;
    }
  }
}

/*!
\brief Compute the curl of the vector field.
\sa Curl(int, int)
*/
ScalarField2 VectorField2::Curl() const
{
  ScalarField2 c(GetBox(), nx, ny);

  for (int i = 0; i < nx; i++)
  {
    for (int j = 0; j < ny; j++)
    {
      c(i, j) = Curl(i, j);
    }
  }

  return c;
}

/*!
\brief Compute the curl at a given sample.
\sa Curl()
\param i,j Integer coordinates of the sample.
*/
double VectorField2::Curl(int i, int j) const
{
  double c = 0.0;

  // Curl along x axis
  if (i == 0)
  {
    c += (at(i + 1, j)[1] - at(i, j)[1]) * inversecelldiagonal[0];
  }
  else if (i == nx - 1)
  {
    c += (at(i, j)[1] - at(i - 1, j)[1]) * inversecelldiagonal[0];
  }
  else
  {
    c += 0.5 * (at(i + 1, j)[1] - at(i - 1, j)[1]) * inversecelldiagonal[0];
  }

  // Curl along y axis
  if (j == 0)
  {
    c -= (at(i, j + 1)[0] - at(i, j)[0]) * inversecelldiagonal[1];
  }
  else if (j == ny - 1)
  {
    c -= (at(i, j)[0] - at(i, j - 1)[0]) * inversecelldiagonal[1];
  }
  else
  {
    c -= 0.5 * (at(i, j + 1)[0] - at(i, j - 1)[0]) * inversecelldiagonal[1];
  }

  return c;
}/*!
\brief Get the field value at a given point.

This function relies on a bi-linear interpolation of the vectors.

\sa Math::Bilinear
\param p Point.
*/
Vector2 VectorField2::Value(const Vector2& p) const
{
  double u, v;
  int i, j;
  CellInteger(p, i, j, u, v);

  // Test position
  if (!InsideCellIndex(i, j))
    return Vector2::Null;

  return Vector2::Bilinear(at(i, j), at(i + 1, j), at(i + 1, j + 1), at(i, j + 1), u, v);
}
