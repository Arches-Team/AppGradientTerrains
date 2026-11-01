// Mesh

#include <charconv>
#include <algorithm>

#include "mesh.h"

#include <QtCore/QRegularExpression>

/*!
\class Mesh mesh.h

\brief Core triangle mesh class.
\ingroup ExtendedKernelGroup
*/

/*!
\brief Initialize the mesh to empty.
*/
Mesh::Mesh()
{}/*!
\brief Create the mesh.

\param vertices, normals Array of vertices and normals.
\param va, na Array of vertex and normal indexes.
*/
Mesh::Mesh(const QVector<Vector>& vertices, const QVector<Vector>& normals, const QVector<int>& va, const QVector<int>& na) :vertices(vertices), normals(normals), varray(va), narray(na)
{}/*!
\brief Empty
*/
Mesh::~Mesh()
{}


/*!
\brief Add a smooth triangle to the geometry.
\param a, b, c Indexes of the vertices.
\param na, nb, nc Indexes of the normals.
*/
void Mesh::AddSmoothTriangle(int a, int na, int b, int nb, int c, int nc)
{
  varray.append(a);
  narray.append(na);
  varray.append(b);
  narray.append(nb);
  varray.append(c);
  narray.append(nc);
}
/*!
\brief Add a triangle to the geometry.
\param a, b, c Indexes of the vertices.
\param n Index of the normal.
*/
void Mesh::AddTriangle(int a, int b, int c, int n)
{
  varray.append(a);
  narray.append(n);
  varray.append(b);
  narray.append(n);
  varray.append(c);
  narray.append(n);
}/*!
\brief Add a smmoth quadrangle to the geometry.

Creates two smooth triangles abc and acd.

\param a, b, c, d  Indexes of the vertexes.
\param na, nb, nc, nd Indexes of the normals for the vertexes.
*/
void Mesh::AddSmoothQuadrangle(int a, int na, int b, int nb, int c, int nc, int d, int nd)
{
  // First triangle
  AddSmoothTriangle(a, na, b, nb, c, nc);

  // Second triangle
  AddSmoothTriangle(a, na, c, nc, d, nd);
}
/*!
\brief Creates an axis aligned box.

The object has 8 vertices, 6 normals and 12 triangles.
\param box The box.
*/
Mesh::Mesh(const Box& box) :vertices(8), normals(6)
{
  // Vertices
  for (int i = 0; i < 8; i++)
  {
    vertices[i] = box.Vertex(i);
  }

  // Normals
  normals[0] = Vector(-1, 0, 0);
  normals[1] = Vector(1, 0, 0);
  normals[2] = Vector(0, -1, 0);
  normals[3] = Vector(0, 1, 0);
  normals[4] = Vector(0, 0, -1);
  normals[5] = Vector(0, 0, 1);

  // Reserve space for the triangle array
  varray.reserve(12 * 3);
  narray.reserve(12 * 3);

  AddTriangle(0, 2, 1, 4);
  AddTriangle(1, 2, 3, 4);

  AddTriangle(4, 5, 6, 5);
  AddTriangle(5, 7, 6, 5);

  AddTriangle(0, 4, 2, 0);
  AddTriangle(4, 6, 2, 0);

  AddTriangle(1, 3, 5, 1);
  AddTriangle(3, 7, 5, 1);

  AddTriangle(0, 1, 5, 2);
  AddTriangle(0, 5, 4, 2);

  AddTriangle(3, 2, 7, 3);
  AddTriangle(6, 7, 2, 3);
}/*!
\brief Creates a sphere, using polar coordinates.

The object has (2 n) (n - 1) + 2 vertices, (2 n) (n - 1) + 2 normals and 4n(n-1) triangles.

\param sphere %Sphere.
\param n Discretization parameter.
*/
Mesh::Mesh(const Sphere& sphere, int n)
{
  Vector c = sphere.Center();
  double r = sphere.Radius();

  // Perimeter
  int p = 2 * n;

  // Size
  int s = (2 * n) * (n - 1) + 2;

  vertices.resize(s);
  normals.resize(s);

  // Create set of vertices
  double dt = Math::Pi / n;
  double df = Math::Pi / n;

  int k = 0;

  // Phi
  double f = -Math::HalfPi;

  for (int j = 1; j < n; j++)
  {
    f += df;
    // Theta
    double t = 0.0;
    for (int i = 0; i < 2 * n; i++)
    {
      Vector u = Vector(cos(t) * cos(f), sin(t) * cos(f), sin(f));
      normals[k] = u;
      vertices[k] = c + r * u;
      k++;
      t += dt;
    }
  }
  // North pole
  normals[s - 2] = Vector(0.0, 0.0, 1.0);
  vertices[s - 2] = c + Vector(0.0, 0.0, r);

  // South
  normals[s - 1] = Vector(0.0, 0.0, -1.0);
  vertices[s - 1] = c - Vector(0.0, 0.0, r);

  // Reserve space for the smooth triangle array
  varray.reserve(4 * n * (n - 1) * 3);
  narray.reserve(4 * n * (n - 1) * 3);

  // South cap
  for (int i = 0; i < 2 * n; i++)
    AddSmoothTriangle(s - 1, s - 1, (i + 1) % p, (i + 1) % p, i, i);

  // North cap
  for (int i = 0; i < 2 * n; i++)
    AddSmoothTriangle(s - 2, s - 2, 2 * n * (n - 2) + i, 2 * n * (n - 2) + i, 2 * n * (n - 2) + (i + 1) % p, 2 * n * (n - 2) + (i + 1) % p);

  // Sphere
  for (int j = 1; j < n - 1; j++)
  {
    for (int i = 0; i < 2 * n; i++)
    {
      AddSmoothQuadrangle((j - 1) * p + i, (j - 1) * p + i, (j - 1) * p + (i + 1) % p, (j - 1) * p + (i + 1) % p, j * p + (i + 1) % p, j * p + (i + 1) % p, j * p + i, j * p + i);
    }
  }
}
#include <QtCore/QFile>
#include <QtCore/QTextStream>




/*!
\brief Merge two meshes into a single one.
\param mesh Argument mesh.
*/
void Mesh::Append(const Mesh& mesh)
{
  // Shifts
  const int v = vertices.size();
  const int n = normals.size();

  // Vertexes
  vertices.append(mesh.vertices);

  // Normals
  normals.append(mesh.normals);

  // Include shifting for vertex and normal indexes
  for (int i = 0; i < mesh.varray.size(); i++)
  {
    varray.append(mesh.varray.at(i) + v);
  }
  for (int i = 0; i < mesh.narray.size(); i++)
  {
    narray.append(mesh.narray.at(i) + n);
  }
}

/*!
\brief Creates an arrow.
The object has 3n+2 vertices, 2n+2 normals and 6n triangles.

Note that this shape is more efficient in terms of storage than a cylinder and a cone.
\param a, b End vertices.
\param r, R %Cylinder and cone radii.
\param l Length of the cylinder, the size of the cone will be derived from the distance <b>b</b>-<b>a</b> minus this length.
\param n Discretization.
*/
Mesh Mesh::Arrow(const Vector& a, const Vector& b, const double& r, const double& R, const double& l, int n)
{
  Vector z = Normalized(b - a);

  // Frame
  Vector x, y;
  z.Orthonormal(x, y);

  Mesh mesh;

  // Set dimension of array of vertices and normals
  mesh.vertices.resize(3 * n + 2);
  mesh.normals.resize(2 * n + 2);

  // Vertices and normals
  for (int i = 0; i < n; i++)
  {
    double t = Math::Angle(i, n);
    Vector u = x * cos(t) + y * sin(t);

    mesh.vertices[i] = a + r * u;
    mesh.vertices[n + i] = a + l * z + r * u;
    mesh.vertices[2 * n + i] = a + l * z + R * u;
    mesh.normals[i] = u;
    mesh.normals[n + i] = Normalized(R * z + (Norm(b - a) - l) * u);
  }

  // End vertices
  mesh.vertices[3 * n] = a;
  mesh.vertices[3 * n + 1] = b;

  // End normals
  mesh.normals[2 * n] = -z;
  mesh.normals[2 * n + 1] = z;

  // Reserve space for the triangle and smooth triangle arrays
  mesh.varray.reserve(6 * n * 3);
  mesh.narray.reserve(6 * n * 3);

  // Lower cap of cylinder
  for (int i = 0; i < n; i++)
  {
    mesh.AddTriangle(3 * n, (i + 1) % n, i, 2 * n);
  }

  // Cone side
  for (int i = 0; i < n; i++)
  {
    mesh.AddSmoothTriangle(2 * n + i, n + i, 2 * n + (i + 1) % n, n + (i + 1) % n, 3 * n + 1, 2 * n + 1);
  }

  // Cylinder side
  for (int i = 0; i < n; i++)
  {
    mesh.AddSmoothTriangle(i, i, (i + 1) % n, (i + 1) % n, n + i, i);
    mesh.AddSmoothTriangle((i + 1) % n, (i + 1) % n, n + (i + 1) % n, (i + 1) % n, n + i, i);
  }

  // Mid cylider closing cap
  for (int i = 0; i < n; i++)
  {
    mesh.AddTriangle(n + i, n + (i + 1) % n, 2 * n + i, 2 * n);
    mesh.AddTriangle(n + (i + 1) % n, 2 * n + (i + 1) % n, 2 * n + i, 2 * n);
  }
  return mesh;
}


#include "arrayinteger.h"

static inline double E(double d, double r)
{
  return Math::Clamp((d - r) / r, -1.0, 1.0);
}