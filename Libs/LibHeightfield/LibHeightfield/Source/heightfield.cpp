// Heightfield

#include "heightfield.h"


Xoshiro256 FlowStruct::fast;

/*!
\mainpage LibHeightField
<center>

<P>Latest stable version : 2024.01.12
</center>
<h3>License</h3>
Copyright &copy; by
<a href="mailto:eric.galin@liris.cnrs.fr">Eric Galin</a>.
No representations are made about the suitability of this software
for any purpose. It is provided as is without express or implied
warranty.

<h3>Fundamental classes in heightfield</h3>
The heightfield library implements several classes for representing heightfields. The list includes two kinds
of classes : \ref HeightField for elevation maps based on regular grids, which inherits from ScalarField2, and \ref AnalyticHeightField which implements several functions for
procedurally defined elevation functions representing terrains.
*/

/*!
\defgroup HeightFieldGroup Heightfield structures and algorithms

\brief Heightfield classes implement several algorithms for processing heightfields.
*/


/*!
\class HeightField heightfield.h
\brief A simple height field.

\ingroup HeightFieldGroup
*/

const double HeightField::flat = 1.0e-8;

/*!
\brief Create a flat heightfield.
\param box Rectangle domain of the terrain.
\param nx, ny Samples.
\param v Constant elevation.
*/
HeightField::HeightField(const Box2& box, int nx, int ny, const double& v) :ScalarField2(box, nx, ny, v)
{}/*!
\brief Create a heightfield from a scalar field.

This constructor provides implicit conversion.

\param s Scalar field.
*/
HeightField::HeightField(const ScalarField2& s) :ScalarField2(s)
{}/*!
\brief Compute the normal at a given sample.

This function uses the weighted sum (area) of the normals of the
triangles sharing the point on the grid. The returned vector is normalized.

\param i,j Integer coordinates of the sample.
*/
Vector HeightField::Normal(int i, int j) const
{
  Vector n;
  if (i == 0)
  {
    if (j == 0)
    {
      // Corner: 0/1
      n = Triangle(Vertex(i, j), Vertex(i + 1, j), Vertex(i + 1, j + 1)).AreaNormal() + Triangle(Vertex(i, j), Vertex(i + 1, j + 1), Vertex(i, j + 1)).AreaNormal();
    }
    else if (j == ny - 1)
    {
      // Corner: 5
      n = Triangle(Vertex(i, j), Vertex(i, j - 1), Vertex(i + 1, j)).AreaNormal();
    }
    else
    {
      // Edge: 0/1/5
      n = Triangle(Vertex(i, j), Vertex(i + 1, j), Vertex(i + 1, j + 1)).AreaNormal() + Triangle(Vertex(i, j), Vertex(i + 1, j + 1), Vertex(i, j + 1)).AreaNormal()
        + Triangle(Vertex(i, j), Vertex(i, j - 1), Vertex(i + 1, j)).AreaNormal();
    }
  }
  else if (i == nx - 1)
  {
    if (j == 0)
    {
      // Corner: 2
      n = Triangle(Vertex(i, j), Vertex(i, j + 1), Vertex(i - 1, j)).AreaNormal();

    }
    else if (j == ny - 1)
    {
      // Corner: 3/4
      n = Triangle(Vertex(i, j), Vertex(i - 1, j - 1), Vertex(i, j - 1)).AreaNormal() + Triangle(Vertex(i, j), Vertex(i - 1, j), Vertex(i - 1, j - 1)).AreaNormal();
    }
    else
    {
      // Edge: 2/3/4
      n = Triangle(Vertex(i, j), Vertex(i, j + 1), Vertex(i - 1, j)).AreaNormal() +
        Triangle(Vertex(i, j), Vertex(i - 1, j), Vertex(i - 1, j - 1)).AreaNormal() +
        Triangle(Vertex(i, j), Vertex(i - 1, j - 1), Vertex(i, j - 1)).AreaNormal();
    }
  }
  else
  {
    if (j == 0)
    {
      // Edge: 0/1/2
      n = Triangle(Vertex(i, j), Vertex(i + 1, j), Vertex(i + 1, j + 1)).AreaNormal() +
        Triangle(Vertex(i, j), Vertex(i + 1, j + 1), Vertex(i, j + 1)).AreaNormal() +
        Triangle(Vertex(i, j), Vertex(i, j + 1), Vertex(i - 1, j)).AreaNormal();
    }
    else if (j == ny - 1)
    {
      // Edge: 3/4/5
      n = Triangle(Vertex(i, j), Vertex(i - 1, j), Vertex(i - 1, j - 1)).AreaNormal() +
        Triangle(Vertex(i, j), Vertex(i - 1, j - 1), Vertex(i, j - 1)).AreaNormal() +
        Triangle(Vertex(i, j), Vertex(i, j - 1), Vertex(i + 1, j)).AreaNormal();
    }
    else
    {
      // Face: 0/1/2/3/4/5
      n = Triangle(Vertex(i, j), Vertex(i + 1, j), Vertex(i + 1, j + 1)).AreaNormal() +
        Triangle(Vertex(i, j), Vertex(i + 1, j + 1), Vertex(i, j + 1)).AreaNormal() +
        Triangle(Vertex(i, j), Vertex(i, j + 1), Vertex(i - 1, j)).AreaNormal() +
        Triangle(Vertex(i, j), Vertex(i - 1, j), Vertex(i - 1, j - 1)).AreaNormal() +
        Triangle(Vertex(i, j), Vertex(i - 1, j - 1), Vertex(i, j - 1)).AreaNormal() +
        Triangle(Vertex(i, j), Vertex(i, j - 1), Vertex(i + 1, j)).AreaNormal();
    }
  }
  return Normalized(n);
}

/*!
\brief Compute the vertex corresponding to a given sample.
\param i, j Integer coordinates of the sample.
*/
Vector HeightField::Vertex(int i, int j) const
{
  return Vector(a[0] + i * celldiagonal[0], a[1] + j * celldiagonal[1], at(i, j));
}/*!
\brief Compute the vertex position on the terrain.
\param p Point.
\param triangular Boolean, use triangular interpolation if set to true, bilinear interpolation otherwize.
*/
Vector HeightField::Vertex(const Vector2& p, bool triangular) const
{
  double u, v;
  int i, j;
  CellInteger(p, i, j, u, v);

  double z = 0.0;
  // Escape
  if (!InsideCellIndex(i, j))
  {
  }
  else
  {
    if (triangular)
    {
      if (u > v)
      {
        z = (1.0 - u) * at(i, j) + (u - v) * at(i + 1, j) + v * at(i + 1, j + 1);
      }
      else
      {
        z = (1.0 - v) * at(i, j) + u * at(i + 1, j + 1) + (v - u) * at(i, j + 1);
      }
    }
    else
    {
      z = Math::Bilinear(at(i, j), at(i + 1, j), at(i + 1, j + 1), at(i, j + 1), u, v);
    }
  }
  return Vector(p[0], p[1], z);
}

#include <QtCore/QStack>

#include <algorithm>

class NeighborPoint
{
public:
  QPoint p; //!< Point.
  double height; //!< Height difference.
  double slope;  //!< Slope.
  double weight;  //!< Weighted slope.
public:
  NeighborPoint(const QPoint& p, double h, double s, double w) :p(p), height(h), slope(s), weight(w) {}
};/*!
\brief Compute the stream area field of the terrain.

The stream area is computed as a fraction of cells, independtly of the surface area of a cell.
\param power Power.
*/
ScalarField2 HeightField::StreamArea(const double& power) const
{
  ScalarField2 stream(Box2(a, b), nx, ny, 1.0);

  QVector<ScalarPoint2> queue = GetScalarPoints();
  std::sort(queue.begin(), queue.end());

  for (int i = queue.size() - 1; i >= 0; i--)
  {
    QPoint p = queue.at(i).Point();

    FlowStruct flow;
    int n = CheckFlowSlope(p, flow, power);
    if (n > 0)
    {
      const double sp = stream(p);
      for (int j = 0; j < n; j++)
      {
        stream(flow.q[j]) += sp * flow.sn[j];
      }
    }
  }

  return stream;
}

/*!
\brief Compute
*/
Vector2 AverageFlowDirection(const QPoint& p, QPoint q[8], double s[8], int n)
{
  Vector2 d = Vector2::Null;
  for (int i = 0; i < n; i++)
  {
    QPoint pq = q[i] - p;
    Vector2 pq2 = Vector2(pq.x(), pq.y());
    d += s[i] * Normalized(pq2);
  }
  return Normalized(d);
}/*!
\brief Get the bounding box of the heightfield.

Although this function has the same name as Array2::GetBox(),
it computes the minimum and maximum elevation of the terrain (computationally intensive).

The two-dimensional box can be obtained by using:
\code
HeightField heightfield;
Box2 box = heightfield.Array2::GetBox(); // Get the domain in the plane.
\endcode

\sa Array2::GetBox()
*/
Box HeightField::GetBox() const
{
  double za, zb;
  GetRange(za, zb);
  return Array2::GetBox().ToBox(za, zb);
}/*!
\brief Compute the intersection between a ray and the surface of the heightfield.

The algorithm uses a ray marching approach, therefore this function may require
many iterations and the resulting intersection may not be accurate.

\param ray The ray.
\param t Returned distance along the ray.
\param box %Box where intersection will be computed.
\param q Returned intersection point.
\param k Lipschitz constant.
\param length Maximum distance along the ray.
\param epsilon Minimum stepping distance.
*/
bool HeightField::Intersect(const Ray& ray, double& t, Vector& q, const Box& box, const double& k, const double& length, const double& epsilon) const
{
  double ta, tb;

  // Check the intersection with the bounding box
  if (!box.Intersect(ray, ta, tb)) return false;

  if (ta<-1.0e8 || tb>+1.0e8) return false;

  tb = Math::Min(tb, length);

  t = Math::Max(ta + epsilon, 0.0);

  // Ray marching
  while (t < tb)
  {
    // Point along the ray
    Vector p = ray(t);

    // Heightfield elevation
    double z = GetHeight(p);
    double h = p[2] - z;
    if (h < epsilon)
    {
      q = Vector(p[0], p[1], z);
      return true;
    }
    else
    {
      t += Math::Max(h / k, epsilon);
    }
  }
  return false;
}/*!
\brief Draw a heightfield.
\param scene Graphics scene.
\param pen The pen.
\param brush The brush.
*/
#pragma warning(push)
#pragma warning(disable: 4100)  
#pragma warning(pop)
#include <QtCore/QRegularExpression>
