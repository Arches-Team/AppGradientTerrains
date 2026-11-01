#pragma once

#include <QtCore/QVector>
#include <QtCore/QSet>

#include "sphere.h"

class PointCurve;
class PointCurve2;
class QuadricCurve;
class QuadricCurveSet;
class CubicCurve;
class CubicCurveSet;
class CubicCurve2Set;
class Circle2;
class Ellipse2;

class Torus;
class Icosidodecahedron;
class Icosahedron;
class Dodecahedron;
class Cuboctahedron;
class Octahedron;
class Pyramid;
class Sphere;
class Hexagonal;
class Cone;
class Rectangles;
class SphereSet;
class Cuboid;
class Voxel;
class Ellipsoid;

class QuadricSurface;

class Mesh2;

class Mesh
{
protected:
  QVector<Vector> vertices; //!< Vertices.
  QVector<Vector> normals;  //!< Normals.
  QVector<int> varray;      //!< Vertex indexes.
  QVector<int> narray;      //!< Normal indexes.
public:
  explicit Mesh();
        explicit Mesh(const QVector<Vector>&, const QVector<Vector>&, const QVector<int>&, const QVector<int>&);
        ~Mesh();

  
  
        
  
  // Number of elements
  int Triangles() const;
    
  
  void Append(const Mesh&);

  QVector<Vector> GetVertices() const;
  QVector<Vector> GetNormals() const;
    
  
  int VertexIndex(int, int) const;
    int NormalIndex(int, int) const;
  
    
    
    
  
    
      
    
        
          
      
      explicit Mesh(const Box&);
    explicit Mesh(const Sphere&, int);
  
            
      
    
  static Mesh Arrow(const Vector&, const Vector&, const double&, const double&, const double&, int);

  
  
  
      
  friend std::ostream& operator<<(std::ostream&, const Mesh&);
protected:
  void AddTriangle(int, int, int, int);
  void AddSmoothTriangle(int, int, int, int, int, int);
  void AddSmoothQuadrangle(int, int, int, int, int, int, int, int);
      
  protected:
          friend class MeshStack;
};




/*!
\brief Get the vertex index of a given triangle.
\param t Triangle index.
\param i Vertex index.
*/
inline int Mesh::VertexIndex(int t, int i) const
{
  return varray.at(t * 3 + i);
}

/*!
\brief Get the normal index of a given triangle.
\param t Triangle index.
\param i Normal index.
*/
inline int Mesh::NormalIndex(int t, int i) const
{
  return narray.at(t * 3 + i);
}










/*!
\brief Get the number of triangles.
*/
inline int Mesh::Triangles() const
{
  return varray.size() / 3;
}



/*!
\brief Get the array of vertices.
*/
inline QVector<Vector> Mesh::GetVertices() const
{
  return vertices;
}

/*!
\brief Get the array of normals.
*/
inline QVector<Vector> Mesh::GetNormals() const
{
  return normals;
}


class ScalarField2;


















