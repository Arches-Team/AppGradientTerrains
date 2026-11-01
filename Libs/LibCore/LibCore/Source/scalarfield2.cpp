// Fields
#include <algorithm>

#include <QtCore/QFile>
#include <QtCore/QTextStream>

#include <immintrin.h>

#include "scalarfield.h"
#include "vectorfield.h"

#include "cpu.h"

/*!
\class ScalarField2 scalarfield.h
\brief A base two-dimensional field of real values.

\ingroup StructureGroup
*/

/*!
\brief Create the field structure.
\param a Array representing the grid domain.
\param v Constant value of field.
*/
ScalarField2::ScalarField2(const Array2& a, const double& v) :Array2(a)
{
  field.fill(v, nx * ny);
}

/*!
\brief Create the field structure.
\param box The box.
\param x,y Size of the array.
\param v Constant value of field.
*/
ScalarField2::ScalarField2(const Box2& box, int x, int y, const double& v) :ScalarField2(Array2(box, x, y), v)
{}
#include <QtGui/QImageReader>
/*!
\brief Empty.
*/
ScalarField2::~ScalarField2()
{}

/*!
\brief Return the array representing the grid domain.
*/
Array2 ScalarField2::GetArray() const
{
  return Array2(Box2(a, b), nx, ny);
}/*!
\brief Get the range of the field.

\avx

\param a,b Returned minimum and maximum.
*/
void ScalarField2::GetRange(double& a, double& b) const
{
  const int size = nx * ny;
  // Escape
  if (size == 0)
  {
    a = b = 0.0;
    return;
  }

#ifdef _MSC_VER
  if (System::Avx() && field.size() > 8)
  {
    const double* p = field.data();

    __m256d avxa = _mm256_load_pd(p);
    __m256d avxb = avxa;

    const int offset = size % 4;
    for (int i = 1; i < size / 4; i++)
    {
      __m256d avxi = _mm256_load_pd(p + i * 4);
      avxa = _mm256_min_pd(avxa, avxi);
      avxb = _mm256_max_pd(avxb, avxi);
    }
    double e[4];
    _mm256_store_pd(e, avxa);
    a = Math::Min(e[0], e[1], e[2], e[3]);

    _mm256_store_pd(e, avxb);
    b = Math::Max(e[0], e[1], e[2], e[3]);

    for (int i = size - offset; i < size; i++)
    {
      double x = field.at(i);
      if (x < a)
      {
        a = x;
      }
      else if (x > b)
      {
        b = x;
      }
    }
  }
  else
#endif
  {
    a = field.at(0);
    b = a;

    for (int i = 1; i < field.size(); i++)
    {
      double x = field.at(i);
      if (x < a)
      {
        a = x;
      }
      else if (x > b)
      {
        b = x;
      }
    }
  }
}
/*!
\brief Create an image from the field.
\param grayscale Export as grayscale if set to true, and to color otherwise (provides a better precision).
*/
QImage ScalarField2::CreateImage(bool grayscale) const
{
  double a, b;
  GetRange(a, b);

  // Check if the scalarfield is constant, in that case modify the range to avoid division by 0 errors
  if (a == b)
  {
    b = a + 1.0;
  }
  return CreateImage(a, b, grayscale);
}/*!
\brief Create an image from the field.
\param a,b Range of elevation that will be mapped to image scale.
\param grayscale Export as grayscale if set to true, color otherwise.
*/
QImage ScalarField2::CreateImage(const double& a, const double& b, bool grayscale) const
{
  // Write Heightmap
  if (grayscale)
  {
    QImage image(nx, ny, QImage::Format_Grayscale16);
    for (int j = 0; j < image.height(); j++)
    {
      quint16* dst = reinterpret_cast<quint16*>(image.bits() + j * image.bytesPerLine());
      for (int i = 0; i < image.width(); i++)
      {
        double x = field.at(VertexIndex(i, j));
        double y = Linear::Step(x, a, b);

        unsigned short pixelval = ushort(y * 65535.0);

        dst[i] = pixelval;
      }
    }
    return image;
  }
  else
  {
    // Define image
    QImage image(nx, ny, QImage::Format_ARGB32);

    // Write Heightmap
    for (int i = 0; i < image.width(); i++)
    {
      for (int j = 0; j < image.height(); j++)
      {
        double x = field.at(VertexIndex(i, j));
        double y = Linear::Step(x, a, b);

        QColor color;

        int c = int(y * (256.0 * 256.0 * 256.0 - 1.0));
        int cr = (c >> 16) & 255;
        int cv = (c >> 8) & 255;
        int cb = c & 255;

        color = QColor(cr, cv, cb);

        image.setPixel(i, j, color.rgb());
      }
    }
    return image;
  }

}/*!
\brief Get the field value at a given point.

This function computes a bi-linear interpolation of the values.

\sa Math::Bilinear
\param p Point (should be strictly inside the box domain).
*/
double ScalarField2::Value(const Vector2& p) const
{
  double u, v;
  int i, j;
  CellInteger(p, i, j, u, v);

  // Test position
  if (!InsideCellIndex(i, j))
  {
    return 0.0;
  }

  return Math::Bilinear(at(i, j), at(i + 1, j), at(i + 1, j + 1), at(i, j + 1), u, v);
}

/*!
\brief Compute the value of the field at the closest grid point.
\param p Point (should be strictly inside the box domain).

\sa Value(const Vector2&),
*/
double ScalarField2::Closest(const Vector2& p) const
{
  double u, v;
  int i, j;
  CellInteger(p, i, j, u, v);

  if (!InsideCellIndex(i, j))
  {
    return 0.0;
  }

  if (u < 0.5 && v < 0.5)
    return at(i, j);
  if (u < 0.5)
    return at(i, j + 1);
  if (v < 0.5)
    return at(i + 1, j);
  return at(i + 1, j + 1);
}

/*!
\brief Get the field value with world coordinate system.

This function computes a triangular interpolation of the values.

\param p Point (should be strictly within bounding box of the domain).
*/
double ScalarField2::Triangular(const Vector2& p) const
{
  double u, v;
  int i, j;
  CellInteger(p, i, j, u, v);

  if (!InsideCellIndex(i, j))
  {
    return 0.0;
  }

  if (u + v < 1.0)
  {
    return (1 - u - v) * at(i, j) + u * at(i + 1, j) + v * at(i, j + 1);
  }
  else
  {
    return (u + v - 1) * at(i + 1, j + 1) + (1 - v) * at(i + 1, j) + (1 - u) * at(i, j + 1);
  }
}

/*!
\brief Get the field value with world coordinate system.

This function computes a bicubic interpolation of the values, with null derivatives.

\sa Math::BiCubic

\param p Point (should be strictly within bounding box of the domain).
*/
double ScalarField2::BiCubic(const Vector2& p) const
{
  double u, v;
  int i, j;
  CellInteger(p, i, j, u, v);

  if (!InsideCellIndex(i, j))
  {
    return 0.0;
  }

  return Math::BiCubic(u, v, at(i, j), at(i + 1, j), at(i + 1, j + 1), at(i, j + 1), 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
}

/*!
\brief Compute the bicubic interpolation.
\author Lois Paulin.
\param p Point (should be strictly within bounding box of the domain).
*/
double ScalarField2::BiCubicValue(const Vector2& p) const
{
  double u, v;
  int i, j;
  CellInteger(p, i, j, u, v);

  //if (!InsideCellIndex(i, j))
  //  return -1.0;

  double a00 = InsideVertexIndex(i, j) ? at(i, j) : 0.0;
  double a01 = InsideVertexIndex(i, j + 1) ? at(i, j + 1) : a00;
  double a10 = InsideVertexIndex(i + 1, j) ? at(i + 1, j) : a00;
  double a11 = InsideVertexIndex(i + 1, j + 1) ? at(i + 1, j + 1) : a00;

  double am10 = InsideVertexIndex(i - 1, j) ? at(i - 1, j) : a00;
  double am11 = InsideVertexIndex(i - 1, j + 1) ? at(i - 1, j + 1) : a01;

  double a0m1 = InsideVertexIndex(i, j - 1) ? at(i, j - 1) : a00;
  double a1m1 = InsideVertexIndex(i + 1, j - 1) ? at(i + 1, j - 1) : a10;

  double a02 = InsideVertexIndex(i, j + 2) ? at(i, j + 2) : a01;
  double a12 = InsideVertexIndex(i + 1, j + 2) ? at(i + 1, j + 2) : a11;

  double a20 = InsideVertexIndex(i + 2, j) ? at(i + 2, j) : a10;
  double a21 = InsideVertexIndex(i + 2, j + 1) ? at(i + 2, j + 1) : a11;

  double am1m1 = InsideVertexIndex(i - 1, j - 1) ? at(i - 1, j - 1) : am10;
  double am12 = InsideVertexIndex(i - 1, j + 2) ? at(i - 1, j + 2) : am11;
  double a2m1 = InsideVertexIndex(i + 2, j - 1) ? at(i + 2, j - 1) : a20;
  double a22 = InsideVertexIndex(i + 2, j + 2) ? at(i + 2, j + 2) : a21;

  am10 = Cubic::Interpolation(v, am1m1, am10, am11, am12);
  a00 = Cubic::Interpolation(v, a0m1, a00, a01, a02);
  a10 = Cubic::Interpolation(v, a1m1, a10, a11, a12);
  a20 = Cubic::Interpolation(v, a2m1, a20, a21, a22);

  return Cubic::Interpolation(u, am10, a00, a10, a20);
}/*!
\brief Sets the entire field with a constant value.
\param s Scalar.
*/
void ScalarField2::Fill(const double& s)
{
  field.fill(s, nx * ny);
}
/*!
\brief Return the field value at a given array vertex.
\param i,j Integer coordinates of the array vertex.
\sa at(int,int)
*/
double ScalarField2::Value(int i, int j) const
{
  return field.at(VertexIndex(i, j));
}

/*!
\brief Compute the gradient at a given array vertex.

\param i,j Integer coordinates of the array vertex.
*/
Vector2 ScalarField2::Gradient(int i, int j) const
{
  Vector2 n;

  // Gradient along x axis
  if (i == 0)
  {
    n[0] = (at(i + 1, j) - at(i, j)) * inversecelldiagonal[0];
  }
  else if (i == nx - 1)
  {
    n[0] = (at(i, j) - at(i - 1, j)) * inversecelldiagonal[0];
  }
  else
  {
    n[0] = (at(i + 1, j) - at(i - 1, j)) * 0.5 * inversecelldiagonal[0];
  }

  // Gradient along y axis
  if (j == 0)
  {
    n[1] = (at(i, j + 1) - at(i, j)) * inversecelldiagonal[1];
  }
  else if (j == ny - 1)
  {
    n[1] = (at(i, j) - at(i, j - 1)) * inversecelldiagonal[1];
  }
  else
  {
    n[1] = (at(i, j + 1) - at(i, j - 1)) * 0.5 * inversecelldiagonal[1];
  }

  return n;
}

/*!
\brief Compute the smooth gradient at a given array vertex using a second order approximation.

\param i,j Integer coordinates of the array vertex.
*/
Vector2 ScalarField2::GradientSmooth(int i, int j) const
{
  Vector2 g;

  if (InsideVertexIndex(i, j, 2))
  {
    g[0] = -at(i - 2, j - 1) - 2.0 * at(i - 1, j - 1) + 2.0 * at(i + 1, j - 1) + at(i + 2, j - 1)
      - 2.0 * at(i - 2, j) - 4.0 * at(i - 1, j) + 4.0 * at(i + 1, j) + 2.0 * at(i + 2, j)
      - at(i - 2, j + 1) - 2.0 * at(i - 1, j + 1) + 2.0 * at(i + 1, j + 1) + at(i + 2, j + 1);

    g[1] = -at(i - 1, j - 2) - 2.0 * at(i - 1, j - 1) + 2.0 * at(i - 1, j + 1) + at(i - 1, j + 2)
      - 2.0 * at(i, j - 2) - 4.0 * at(i, j - 1) + 4.0 * at(i, j + 1) + 2.0 * at(i, j + 2)
      - at(i + 1, j - 2) - 2.0 * at(i + 1, j - 1) + 2.0 * at(i + 1, j + 1) + at(i + 1, j + 2);

    g *= inversecelldiagonal[0] / 32.0;
  }
  else
  {
    g = Gradient(i, j);
  }
  return g;
}

/*!
\brief Compute the local neighborhood in the one-ring aroung a point.
\param i, j Point.
*/
Matrix ScalarField2::Local(int i, int j) const
{
  // Borders
  int ia = (i == 0) ? 0 : i - 1;
  int ib = (i == nx - 1) ? nx - 1 : i + 1;

  int ja = (j == 0) ? 0 : j - 1;
  int jb = (j == ny - 1) ? ny - 1 : j + 1;

  // a(1 + di, 1 + dj) = at(i + di, j + dj), listed in storage order (x varies fastest)
  return Matrix(
    at(ia, ja), at(i, ja), at(ib, ja),
    at(ia, j), at(i, j), at(ib, j),
    at(ia, jb), at(i, jb), at(ib, jb));
}

/*!
\brief Compute the hessian at a given array vertex.

\param i,j Integer coordinates of the array vertex.
*/
Matrix2 ScalarField2::Hessian(int i, int j) const
{
  // Cell size along x and y axes, cells may not be square
  const double dx = celldiagonal[0];
  const double dy = celldiagonal[1];

  // Compute local neighborhood
  Matrix a = Local(i, j);

  return Matrix2(
    (a(2, 1) + a(0, 1) - 2.0 * a(1, 1)) / (dx * dx),
    (a(1, 2) + a(1, 0) - 2.0 * a(1, 1)) / (dy * dy),
    ((a(2, 2) - a(0, 2)) - (a(2, 0) - a(0, 0))) / (4.0 * dx * dy));
}

/*!
\brief Compute the gradient field.
*/
VectorField2 ScalarField2::Gradient() const
{
  VectorField2 v(GetBox(), nx, ny);
  for (int j = 0; j < ny; j++)
  {
    for (int i = 0; i < nx; i++)
    {
      v(i, j) = Gradient(i, j);
    }
  }
  return v;
}/*!
\brief Compute the %Laplacian at a given sample.

Returns a discrete approximation of Laplace�s differential operator applied to the field.
The implementation is equivalent to the 4*del2(U) Matlab function, except on the edges of the field.

\sa Laplacian()
\author Mathieu Gaillard
\param i,j Integer coordinates of the sample.
*/
double ScalarField2::Laplacian(int i, int j) const
{
  double laplacian = 0.0;

  // Divergence along x axis
  if (i == 0)
  {
    laplacian += (at(i, j) - 2.0 * at(i + 1, j) + at(i + 2, j)) / (celldiagonal[0] * celldiagonal[0]);
  }
  else if (i == nx - 1)
  {
    laplacian += (at(i, j) - 2.0 * at(i - 1, j) + at(i - 2, j)) / (celldiagonal[0] * celldiagonal[0]);
  }
  else
  {
    laplacian += (at(i + 1, j) - 2.0 * at(i, j) + at(i - 1, j)) / (celldiagonal[0] * celldiagonal[0]);
  }

  // Divergence along y axis
  if (j == 0)
  {
    laplacian += (at(i, j) - 2.0 * at(i, j + 1) + at(i, j + 2)) / (celldiagonal[1] * celldiagonal[1]);
  }
  else if (j == ny - 1)
  {
    laplacian += (at(i, j) - 2.0 * at(i, j - 1) + at(i, j - 2)) / (celldiagonal[1] * celldiagonal[1]);
  }
  else
  {
    laplacian += (at(i, j + 1) - 2.0 * at(i, j) + at(i, j - 1)) / (celldiagonal[1] * celldiagonal[1]);
  }

  return laplacian;
}

/*!
\brief Compute the %Laplacian field.

Returns a discrete approximation of Laplace�s differential operator applied to the field.
The implementation is equivalent to the 4*del2(U) Matlab function, except on the edges of the field.

\sa Laplacian(int, int)
\author Mathieu Gaillard
*/
ScalarField2 ScalarField2::Laplacian() const
{
  ScalarField2 L(GetBox(), nx, ny);

  for (int i = 0; i < nx; i++)
  {
    for (int j = 0; j < ny; j++)
    {
      L(i, j) = Laplacian(i, j);
    }
  }

  return L;
}/*!
\brief Compute the square root of the scalar field.

\avx
*/
ScalarField2 ScalarField2::Sqrt() const
{
  // Scalar field of the same size
  ScalarField2 r(GetBox(), nx, ny);

  const int size = nx * ny;

#ifdef _MSC_VER
  if (System::Avx())
  {
    double* sp = r.field.data();
    const double* p = field.data();

    const int offset = size % 4;
    for (int i = 0; i < size / 4; i++)
    {
      __m256d avx = _mm256_load_pd(p + i * 4);
      __m256d sqrtavx = _mm256_sqrt_pd(avx);
      _mm256_store_pd(sp + i * 4, sqrtavx);
    }

    for (int i = size - offset; i < size; i++)
    {
      r[i] = sqrt(at(i));
    }
  }
  else
#endif
  {

    for (int i = 0; i < size; i++)
    {
      r[i] = sqrt(at(i));
    }
  }

  return r;
}/*!
\brief Compute the square root of the scalar field.

This function changes the values of the  field.

\sa ScalarField2::Sqrt() const
\avx
*/
void ScalarField2::Sqrted()
{
  const int size = nx * ny;

#ifdef _MSC_VER
  if (System::Avx())
  {
    double* p = field.data();

    const int offset = size % 4;
    for (int i = 0; i < size / 4; i++)
    {
      __m256d avx = _mm256_load_pd(p + i * 4);
      __m256d sqrtavx = _mm256_sqrt_pd(avx);
      _mm256_store_pd(p + i * 4, sqrtavx);
    }

    for (int i = size - offset; i < size; i++)
    {
      field[i] = sqrt(at(i));
    }
  }
  else
#endif
  {
    for (int i = 0; i < size; i++)
    {
      field[i] = sqrt(at(i));
    }
  }
}

/*!
\brief Compute the Lipschitz constant of the elevation function.

Note that this is equivalent to the following code, with the
difference that this function does not store the norm of the gradient in memory.
\code
ScalarField2 n=f.GradientNorm();
double k=0.0;
for (int i=0;i<f.VertexSize();i++)
{
k=Math::Max(k,f.at(i));
}
\endcode
*/
double ScalarField2::K() const
{
  double k = 0.0;

  for (int i = 0; i < nx; i++)
  {
    for (int j = 0; j < ny; j++)
    {
      k = Math::Max(k, Norm(Gradient(i, j)));
    }
  }
  return k;
}

/*!
\brief Normalize the values of a scalar field to unit interval.

Compute range and apply the affine transformation to map values to [0,1].

\sa Unitize()
\avx
*/
void ScalarField2::Normalize()
{
  const int size = nx * ny;
  const int offset = size % 4;

  // Escape
  if (size == 0)
    return;

  double a, b;
  GetRange(a, b);
  if (a == b)
  {
    field.fill(1.0);
    return;
  }
#ifdef _MSC_VER
  if (System::Avx())
  {
    double* p = field.data();

    __m256d avxiba = _mm256_set1_pd(1.0 / (b - a));
    __m256d avxa = _mm256_set1_pd(a);

    for (int i = 0; i < size / 4; i++)
    {
      __m256d avx = _mm256_load_pd(p + i * 4);
      // Could use _mm256_fmadd_ps
      avx = _mm256_sub_pd(avx, avxa);
      avx = _mm256_mul_pd(avx, avxiba);
      _mm256_store_pd(p + i * 4, avx);
    }

    for (int i = size - offset; i < size; i++)
    {
      field[i] = (field[i] - a) / (b - a);
    }
  }
  else
#endif
  {
    for (int i = 0; i < field.size(); i++)
    {
      field[i] = (field[i] - a) / (b - a);
    }
  }
}
#include <math.h>
/*!
\brief Scale the values of a scalar field.

\param s Scaling factor.
*/
ScalarField2& ScalarField2::operator*=(const double& s)
{
  const int size = nx * ny;

  for (int i = 0; i < size; i++)
  {
    field[i] *= s;
  }
  return *this;
}/*!
\brief Addition.
\param s Scalar field.
\avx
*/
ScalarField2& ScalarField2::operator+=(const ScalarField2& s)
{
#ifdef _MSC_VER
  if (System::Avx())
  {
    const double* sp = s.field.data();
    double* p = field.data();

    const int size = nx * ny;
    const int offset = size % 4;
    for (int i = 0; i < size / 4; i++)
    {
      __m256d avx = _mm256_load_pd(p + i * 4);
      __m256d savx = _mm256_load_pd(sp + i * 4);
      __m256d addavx = _mm256_add_pd(avx, savx);
      _mm256_store_pd(p + i * 4, addavx);
    }

    for (int i = size - offset; i < size; i++)
    {
      field[i] += s.field.at(i);
    }
  }
  else
#endif
  {
    int n = field.size();
    double* fieldp = field.data();
    const double* sfieldp = s.field.data();

    for (int i = 0; i < n; i++)
    {
      fieldp[i] += sfieldp[i];
    }
  }
  return *this;
}

/*!
\brief Perform a linear step over the values of a scalar field.

The function Linear::Step() is applied to all the values of the field.
\sa Linear::Step()
\sa ScalarField2::Normalize()

\param a,b Interval.
*/
void ScalarField2::Step(const double& a, const double& b)
{
  // The step could use _mm256_fmadd_pd with field.at(i), 1.0/(b-a) and -a/(b-a)
  // or _mm256_fadd_pd with -a and then _mm256_fmull_pd with 1.0/(b-a)
  // Finally, clamp to [0,1] by using _mm256_max_pd and _mm256_min_pd

  const int size = nx * ny;

  for (int i = 0; i < size; i++)
  {
    field[i] = Linear::Step(field.at(i), a, b);
  }
}/*!
\brief Scales the scalar field to a given range interval.

\param a,b Interval.
*/
void ScalarField2::SetRange(const double& a, const double& b)
{
  double x, y;
  GetRange(x, y);
  if (x == y)
  {
    field.fill(a, field.size());
  }
  else
  {
    const double c = (b - a) / (y - x);
    for (int i = 0; i < field.size(); i++)
    {
      field[i] = a + c * (field.at(i) - x);
    }
  }
}/*!
\brief Create the set of scalar points.
*/
QVector<ScalarPoint2> ScalarField2::GetScalarPoints() const
{
  // Create array
  QVector<ScalarPoint2> e(nx * ny);

  int k = 0;
  for (int i = 0; i < nx; i++)
  {
    for (int j = 0; j < ny; j++)
    {
      e[k++] = ScalarPoint2(QPoint(i, j), at(i, j));
    }
  }
  return e;
}
static double* dt1d(double* f, int n)
{
  double* d = new double[n];
  int* v = new int[n];
  double* z = new double[n + 1];
  int k = 0;

  v[0] = 0;
  z[0] = -Math::Infinity;
  z[1] = Math::Infinity;
  for (int q = 1; q <= n - 1; q++)
  {
    double s = ((f[q] + q * q) - (f[v[k]] + v[k] * v[k])) / double(2 * q - 2 * v[k]);
    while (s <= z[k])
    {
      k--;
      s = ((f[q] + q * q) - (f[v[k]] + v[k] * v[k])) / double(2 * q - 2 * v[k]);
    }
    k++;
    v[k] = q;
    z[k] = s;
    z[k + 1] = Math::Infinity;
  }

  k = 0;
  for (int q = 0; q <= n - 1; q++)
  {
    while (z[k + 1] < q)
    {
      k++;
    }
    d[q] = (q - v[k]) * (q - v[k]) + f[v[k]];
  }

  delete[] v;
  delete[] z;
  return d;
}

/*
\brief Compute the squared distance field from a scalar field.

Points with values below the threshold value are considered as inside, others as outside.
\param threshold Threshold value defining inside and outside.
*/
ScalarField2 ScalarField2::DistanceTransform(const double& threshold) const
{
  double* f = new double[max(nx, ny)];
  ScalarField2 dist(GetArray());

  for (int x = 0; x < nx; x++)
  {
    for (int y = 0; y < ny; y++)
    {
      dist(x, y) = at(x, y) < threshold ? at(x, y) : Math::Large;
    }
  }

  // Transform along columns
  for (int x = 0; x < nx; x++)
  {
    for (int y = 0; y < ny; y++)
    {
      f[y] = dist(x, y);
    }
    double* d = dt1d(f, ny);
    for (int y = 0; y < ny; y++)
    {
      dist(x, y) = d[y];
    }
    delete[] d;
  }

  // Transform along rows
  for (int y = 0; y < ny; y++)
  {
    for (int x = 0; x < nx; x++)
    {
      f[x] = dist(x, y);
    }
    double* d = dt1d(f, nx);
    for (int x = 0; x < nx; x++)
    {
      dist(x, y) = d[x];
    }
    delete[] d;
  }

  return dist;
}
