#include "mayageometry.h"

#include <QtCore/QFile>


/*!
\mainpage MayaLib
<center>
\image html maya-small.png
<P>Latest stable version : 2024.01.12
</center>
<h3>License</h3>
Copyright &copy; by
<a href="mailto:eric.galin@liris.cnrs.fr">Eric Galin</a>, <a href="mailto:adrien.peytavie@liris.cnrs.fr">Adrien Peytavie</a>, <a href="mailto:eric.guerin@liris.cnrs.fr">Eric Gu�rin</a>.
No representations are made about the suitability of this software
for any purpose. It is provided as is without express or implied
warranty.
<h3>Features overview</h3>
This library implements core classes for handling meshes.
The overall architecture of the system is as follows:
\image html architecture.png

<h2>Fundamentals</h2>

The Maya library is built upon a set of core classes, provided by the Core library. These include the following:
-# The MayaGeometry classes, a set of classes for storing the geometry of objects.
-# The MayaInstance classes for handling objects stored on the disc and identified by their name.
-# The MayaGpu classes for handling objects stored on the graphics card.

<h2>Graphical User Interface</h2>

The Maya library includes support for Qt development with QtGLWidget.
This class provides a simple user interface for visualizing geometric objects defined using MayaGeometry or MayaInstance elements.

*/

/*!
\defgroup MayaCore Maya geometry core classes

\brief Maya core geometric classes include several core classes such as MayaGeometry, MayaInstance,
MayaInstanceSet and many others that are usefull for handling meshes.
*/

/*!
\defgroup MayaGpuGroup Maya GPU classes

\brief Maya GPU classes implement Vertex Buffer Objects, shading and intancing on the GPU.
*/

/*!
\defgroup MayaInstanceGroup Maya instance classes

\brief Maya instance classes implement a framework for working with named instances.
MayaInstance, MayaInstanceSet and MayaInstanceAll classes are used in the MayaResources
which manages atlases of assets referenced by their names.
*/

/*!
\class MayaGeometry maya.h
\brief A simple triangle mesh representation.

This class has been designed for the manipulation of geometric instances.

Example of a MayaGeometry that stores a tetrahedron:
\code
QVector<Vector> vertex;
QVector<Vector> normal;
QVector<int> index;
vertex.append(Vector(0,0,0));
vertex.append(Vector(1,0,0));
vertex.append(Vector(0,1,0));
vertex.append(Vector(0,0,1));
normal.append(Vector(-1,0,0));
normal.append(Vector(0,-1,0));
normal.append(Vector(0,0,-1));
normal.append(Normalized(Vector(1,1,1)));
// Set the indexes defining the oblique triangle
index.append(1); // Index of first vertex
index.append(3); // Oblique normal
index.append(2); // Second
index.append(3); // Same oblique normal
index.append(3); // Third
index.append(3); // Again the same normal
//  and so on for the other triangles
MayaGeometry object("tetrahedron",vertex,normal,index);
\endcode
Note that in this case, the vertices and normals are uniquely defined
and correctly shared.
Another possibility is to create the tetrahedron by adding triangles
directly to the MayaGeometry structure. In that case however, the
object is a triangle soup: points (and normals) are replicated.
\code
MayaGeometry object("tetrahedron");
object.AddTriangle(Vector(1,0,0),Vector(0,1,0),Vector(0,0,1)); // ... and so on for the other triangles
\endcode
\ingroup MayaCore
*/

/*!
\brief Creates an geometry from a mesh.
\param name Name of the object.
\param mesh The mesh.
\param mo Material.
*/
MayaGeometry::MayaGeometry(const QString& name, const Mesh& mesh, const MayaMaterial& mo) :name(name), mat(mo)
{
  MayaGeometry::vertex = mesh.GetVertices();
  MayaGeometry::normal = mesh.GetNormals();

  indexes.resize(mesh.Triangles() * 3);

  for (int i = 0; i < mesh.Triangles(); i++)
  {
    indexes[i * 3 + 0] = MayaIndexVertexData(mesh.VertexIndex(i, 0), mesh.NormalIndex(i, 0));
    indexes[i * 3 + 1] = MayaIndexVertexData(mesh.VertexIndex(i, 1), mesh.NormalIndex(i, 1));
    indexes[i * 3 + 2] = MayaIndexVertexData(mesh.VertexIndex(i, 2), mesh.NormalIndex(i, 2));
  }
}/*!
\brief Creates an empty mesh structure.
\param name Name of the object.
\param mo Material.
*/
MayaGeometry::MayaGeometry(const QString& name, const MayaMaterial& mo) :name(name), mat(mo)
{}

/*!
\brief Empty.
*/
MayaGeometry::~MayaGeometry()
{}/*!
\brief Creates a mesh given an array of vertices, normals and colors, and an array of integers defining the (possibly smooth) triangles of the mesh.
\param name Name of the object.
\param vertex Array of vertices.
\param normal Array of normals.
\param UVmap  Array of UVmaps.
\param index Indexes.
\param mo Material.
*/
MayaGeometry::MayaGeometry(const QString& name, const QVector<Vector>& vertex, const QVector<Vector>& normal, const QVector<Vector2>& UVmap, const QVector<MayaIndexVertexData>& index, const MayaMaterial& mo) :name(name), mat(mo)
{
  MayaGeometry::vertex = vertex;
  MayaGeometry::normal = normal;
  MayaGeometry::UVmap = UVmap;
  MayaGeometry::indexes = index;

  MayaGeometry::mat.materialNode = UVMapping;
}/*!
\brief Merges an argument mesh with the existing mesh.

Note that this function may be slow as it may require
a time consuming copy of several arrays.

\param mesh Geometry that will be merged.
*/
void MayaGeometry::Merge(const MayaGeometry& mesh)
{
  // Set size
  vertex.resize(vertex.size() + mesh.vertex.size());
  normal.resize(normal.size() + mesh.normal.size());
  color.resize(color.size() + mesh.color.size());
  UVmap.resize(UVmap.size() + mesh.UVmap.size());

  indexes.resize(indexes.size() + mesh.indexes.size());

  // Merge vertices, normals, colors, UVCoord 
  for (int i = 0; i < mesh.vertex.size(); i++)
  {
    vertex[vertex.size() - mesh.vertex.size() + i] = mesh.vertex[i];
  }
  for (int i = 0; i < mesh.normal.size(); i++)
  {
    normal[normal.size() - mesh.normal.size() + i] = mesh.normal[i];
  }
  for (int i = 0; i < mesh.color.size(); i++)
  {
    color[color.size() - mesh.color.size() + i] = mesh.color[i];
  }
  for (int i = 0; i < mesh.UVmap.size(); i++)
  {
    UVmap[UVmap.size() - mesh.UVmap.size() + i] = mesh.UVmap[i];
  }

  // Indexes should be shifted
  for (int i = 0; i < mesh.indexes.size(); i++)
  {
    MayaIndexVertexData mi = mesh.indexes[i];
    mi.setIV(mi.getIV() + vertex.size() - mesh.vertex.size());
    mi.setIN(mi.getIN() + normal.size() - mesh.normal.size());
    mi.setIC(mi.getIC() != -1 ? (mi.getIC() + color.size() - mesh.color.size()) : -1);
    mi.setIUV(mi.getIUV() != -1 ? (mi.getIUV() + UVmap.size() - mesh.UVmap.size()) : -1);

    indexes[indexes.size() - mesh.indexes.size() + i] = mi;
  }
}/*!
\brief Transforms the geometry given a transformation operator.
\param frame Transformation.
*/
MayaGeometry& MayaGeometry::Transform(const FrameScaled& frame)
{
  // Apply transformation to vertices
  for (int i = 0; i < vertex.size(); i++)
  {
    vertex[i] = frame.Transform(vertex[i]);
  }

  // Compute normal
  for (int i = 0; i < normal.size(); i++)
  {
    normal[i] = Normalized(frame.TransformDirection(normal[i]));
  }

  return *this;
}/*!
\brief Compute the bounding box of the object.
*/
Box MayaGeometry::GetBox() const
{
  int n = vertex.size();
  if (n > 0)
  {
    // Starting box
    Box box = Box(vertex[0]);

    // Extend
    for (int i = 1; i < n; i++)
    {
      box.Extend(vertex[i]);
    }

    return box;
  }
  else
  {
    return Box::Null;
  }
}

/*!
\brief Hash of the vertices, normals, colors, UVs and indexes, used to detect an unchanged geometry.
*/
size_t MayaGeometry::GetDataKey() const
{
  static_assert(sizeof(Vector) == 3 * sizeof(double) && sizeof(Vector2) == 2 * sizeof(double) && sizeof(MayaIndexVertexData) == 4 * sizeof(int), "Arrays are hashed as raw memory, which must have no padding");

  size_t key = qHashMulti(0, vertex.size(), normal.size(), color.size(), UVmap.size(), indexes.size());
  key = qHashBits(vertex.constData(), vertex.size() * sizeof(Vector), key);
  key = qHashBits(normal.constData(), normal.size() * sizeof(Vector), key);
  key = qHashBits(color.constData(), color.size() * sizeof(Vector), key);
  key = qHashBits(UVmap.constData(), UVmap.size() * sizeof(Vector2), key);
  key = qHashBits(indexes.constData(), indexes.size() * sizeof(MayaIndexVertexData), key);
  return key;
}

/*!
\brief Compute the statistics of the object.

This member computes the number of vertices and triangles of the object.
Since vertices may not be shared, those numbers may be larger than the real
number of vertices of the geometric object.

\return The statistics.
*/
MayaStatistics MayaGeometry::GetStatistics() const
{
  return MayaStatistics(1, vertex.size(), indexes.size() / 3, 0, 0, 0);
}


/*!
\brief Apply a planar mapping onto the object.

Delete ancient UVmaps.
\param box Domain.
*/
void MayaGeometry::generatePlannarZ_Mapping(Box2 box)
{
  UVmap.clear();
  Vector2 size = box.Size();

  for (int i = 0; i < vertex.size(); i++)
  {
    UVmap << Vector2((vertex[i][0] - box[0][0]) / size[0], 1.0 - (vertex[i][1] - box[0][1]) / size[1]);
  }

  for (int i = 0; i < indexes.size(); i++)
  {
    indexes[i].setIUV(indexes[i].getIV());
  }
}