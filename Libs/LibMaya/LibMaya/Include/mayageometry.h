// Maya  

#pragma once

#include <QtCore/QTextStream> 
#include <QtCore/QVector>
#include <QtCore/QMap>


#include "color.h" 
#include "vectorfield.h"

#include "mesh.h"

class Voxel;

class MayaStatistics {
protected:
  int s[6]; //!< %Array of integers storing statistics.
public:
  explicit MayaStatistics(int = 0, int = 0, int = 0, int = 0, int = 0, int = 0);
  
  int GetObjects() const;
  int GetObjectsVertices() const;
  int GetObjectsTriangles() const;
  int GetInstances() const;
  int GetInstancesVertices() const;
  int GetInstancesTriangles() const;

  // Assignment operators
  MayaStatistics& operator+= (const MayaStatistics&);

  // Binary operators for combining statistics
  friend MayaStatistics operator+ (const MayaStatistics&, const MayaStatistics&);

  
  // Utilities
  void ClearInstances();
  void AddInstances(int = 1);
};

//! Destructive addition.
inline MayaStatistics& MayaStatistics::operator+= (const MayaStatistics& statistics)
{
  for (int i = 0; i < 6; i++)
  {
    s[i] += statistics.s[i];
  }

  return *this;
}

/*!
\brief Return the number of unique objects.
*/
inline int MayaStatistics::GetObjects() const
{
  return s[0];
}
/*!
\brief Return the number of vertices of unique objects.
*/
inline int MayaStatistics::GetObjectsVertices() const
{
  return s[1];
}
/*!
\brief Return the number of triangles of unique objects.
*/
inline int MayaStatistics::GetObjectsTriangles() const
{
  return s[2];
}
/*!
\brief Return the number of instances.
*/
inline int MayaStatistics::GetInstances() const
{
  return s[3];
}
/*!
\brief Return the number of vertices.
*/
inline int MayaStatistics::GetInstancesVertices() const
{
  return s[4];
}
/*!
\brief Return the number of triangless.
*/
inline int MayaStatistics::GetInstancesTriangles() const
{
  return s[5];
}

class MayaIndexVertexData
{
protected:
  int iVertex;//!< Index of Vertex.
  int iNormal;//!< Index of Normal
  int iColor; //!< Index of Color
  int iUVCoord;//!< Index of UVCoord

public:
  MayaIndexVertexData(int = -1, int = -1, int = -1, int = -1);
  
  // Setters
  void setIV(int i) { iVertex = i; }
  void setIN(int i) { iNormal = i; }
  void setIC(int i) { iColor = i; }
  void setIUV(int i) { iUVCoord = i; }

  // Getters
  int getIV() const { return iVertex; }
  int getIN() const { return iNormal; }
  int getIC() const { return iColor; }
  int getIUV() const { return iUVCoord; }
};


/*!
\brief Create a index structure.
\param v, c, n, uv Vertex, normal, color and texture indexes.
*/
inline MayaIndexVertexData::MayaIndexVertexData(int v, int n, int c, int uv)
{
  iVertex = v;
  iNormal = n;
  iColor = c;
  iUVCoord = uv;
}

enum ShaderNode
{
  ShaderPhong = 0,
  ShaderPhongVertexColor,
  ShaderTextureUV,
  ShaderTerrainTexture3D,
  ShaderTerrainIGN,
  ShaderToon,
  ShaderGooch,
  ShaderGrid,
  ShaderWireframe,
  ShaderNormal,
  ShaderBumpNormal,
  ShaderThinSnow,
  ShaderGreyCoolWarm,
  ShaderGreyCoolWarmWireframe,
  ShaderWireframeAspectRatio,
  ShaderAxelTerrain,

  // Wireframe
  ShaderPhongWireframe,
  ShaderPhongVertexColorWireframe,
  ShaderTextureUVWireframe,
  ShaderTerrainTexture3DWireframe,
  ShaderTerrainIGNWireframe,
  ShaderToonWireframe,
  ShaderGoochWireframe,
  ShaderGridWireframe,
  ShaderNormalWireframe,
  ShaderBumpNormalWireframe,
  ShaderThinSnowWireframe,

  // Triplanar
  ShaderTriPlanar
};

enum MaterialNode
{
  None = 0,
  VertexColor,
  UVMapping
};


// PBR Texture https://www.a23d.co/blog/different-maps-in-pbr-textures/
class MayaTexturePBR {
public:
  QImage albedo;
  QImage normal;
  QImage opacity;

  QImage roughness;
  QImage metalness;
  QImage specular;
  QImage height;
  QImage ambientOcclusion;
  QImage refraction;
  QImage emissive;

public:
  };

class MayaMaterial
{
public:
  ShaderNode shaderNode;

  QString name;         //!< name of the material (for export purpose)

  Color ambient = Color::Black;				//!< Ambient color.
  Color diffuse = Color::Black;				//!< Diffuse color.
  Color specular = Color::Black;				//!< Specular color.
  double shininess = 1.0;			//!< Specular exponent.

  MayaTexturePBR texture;

  MaterialNode materialNode;
public:
  //! Empty
  MayaMaterial() {}
  MayaMaterial(ShaderNode, const Color&, const Color&, const Color & = Color(0.1, 0.1, 0.1, 1.0), const double& = 20.0, const QImage = QImage(), const QImage & = QImage());
  
  //! Empty
  ~MayaMaterial() {}

  
  public:
  static MayaMaterial None;						//!< Default material.
  static MayaMaterial Normal;					//!< Normal shading based material.
  static MayaMaterial NormalWire;				//!< Normal shading based material.
  static MayaMaterial MayaTransparent;			//!< Else.
  static MayaMaterial MayaTransparentWire;		//!< Else.
  static MayaMaterial WireframeAspectRatio;		//!< A Wireframe shading with an emphasis on the degenerate triangles (in red).
  static MayaMaterial GreenColor;				//!< Normal shading based material.
  static MayaMaterial YellowColor;				//!< Normal shading based material.
  static MayaMaterial RedColor;				//!< Normal shading based material.
  static MayaMaterial BlueColor;				//!< Normal shading based material.
  static MayaMaterial ColdWarm;				//!< Gooch shading based material.
  static MayaMaterial TriPlanarRock;					//!< Rock.
  static MayaMaterial TriPlanar;					//!< Tri-planar projection.
};

// Forward declaration
class MayaGeometryAll;

// Base geometry class
class MayaGeometry
{
public:
  QString name;            //!< Signature of the object.
  QVector<Vector> vertex;  //!< %Array of vertices.
  QVector<Vector> normal;  //!< %Array of normals.
  QVector<Vector> color;   //!< %Array of colors.
  QVector<Vector2> UVmap;   //!< %Array of UVs.
  QVector<MayaIndexVertexData> indexes; //!< %Array of integers referencing vertices, normals, colors and UV. 
  MayaMaterial mat;        //!< Material of the object.

public:
  // Constructors
  explicit MayaGeometry(const QString & = QString("None"), const MayaMaterial & = MayaMaterial::None);
      MayaGeometry(const QString&, const QVector<Vector>&, const QVector<Vector>&, const QVector<Vector2>&, const QVector<MayaIndexVertexData>&, const MayaMaterial & = MayaMaterial::None);
  MayaGeometry(const QString&, const Mesh&, const MayaMaterial & = MayaMaterial::None);
          ~MayaGeometry();

  
    
  void SetName(const QString&);
  void SetMaterial(const MayaMaterial&);

  void Merge(const MayaGeometry&);

    
  QString GetName() const;
  MayaMaterial GetMaterial() const;
  Box GetBox() const;
  size_t GetDataKey() const;

  MayaGeometry& Transform(const FrameScaled&);
  
  
  
          bool Save_OBJ(QTextStream&, int = 1, int = 1, int = 1) const;
    
  MayaStatistics GetStatistics() const;
    const Vector& GetVertex(int, int) const;
    const Vector& GetNormal(int, int) const;
    const Vector& GetColor(int, int) const;
  
  const Vector2& GetUV(int, int) const;
  
  int size_vertex() const { return vertex.size(); }
  int size_normal() const { return normal.size(); }
  int size_UV() const { return UVmap.size(); }
    int size_triangles() const { return indexes.size() / 3; }

    
  
  // Generate Mapping
  void generatePlannarZ_Mapping(Box2);
  private:
  
protected:
      };

/*!
\brief Get the k-th vertex of the i-th triangle.
\param i Triangle number.
\param k Vertex.
*/
inline const Vector& MayaGeometry::GetVertex(int i, int k) const
{
  return vertex.at(indexes.at(i * 3 + k).getIV());
}

/*!
\brief Get the normal of the k-th vertexof the i-th triangle.
\param i Triangle number.
\param k Vertex.
*/
inline const Vector& MayaGeometry::GetNormal(int i, int k) const
{
  return normal.at(indexes.at(i * 3 + k).getIN());
}

/*!
\brief Get the color of the k-th vertex of the i-th triangle.
\param i Triangle number.
\param k Vertex.
*/
inline const Vector& MayaGeometry::GetColor(int i, int k) const
{
  return color.at(indexes.at(i * 3 + k).getIC());
}







/*!
\brief Get the i-th UV in the array.
\param i,k Indexes.
*/
inline const Vector2& MayaGeometry::GetUV(int i, int k) const
{
  return UVmap.at(indexes.at(i * 3 + k).getIUV());
}

/*!
\brief Set the name.
\param n Name.
*/
inline void MayaGeometry::SetName(const QString& n)
{
  name = n;
}

/*!
\brief Set the MayaMaterial.
\param mo MayaMaterial.
*/
inline void MayaGeometry::SetMaterial(const MayaMaterial& mo)
{
  mat = mo;
}
/*!
\brief Get the name of the geometry.
*/
inline QString MayaGeometry::GetName() const
{
  return name;
}

/*!
\brief Get the material of the geometry.
*/
inline MayaMaterial MayaGeometry::GetMaterial() const
{
  return mat;
}

// Set of Maya Geometry Instances
class MayaGeometrySet : public MayaGeometry
{
protected:
  QVector<FrameScaled> frames; //!< %Array of frames
public:
  //! Empty
  MayaGeometrySet() {}
    
    MayaGeometrySet(const MayaGeometry&, const FrameScaled&);
    
  //! Empty
  ~MayaGeometrySet() {}

  // Setters
  void Append(const FrameScaled&);
        
      
    
  // Getters
  QString GetName() const;
    FrameScaled GetFrameScaled(int) const;
  QVector<FrameScaled> GetFrames() const;
  int count() const;

  MayaGeometry Collapse() const;
          

  MayaStatistics GetStatistics() const;
  
  };


/*!
\brief Get the i-th frame.
\param i Index.
*/
inline FrameScaled MayaGeometrySet::GetFrameScaled(int i) const
{
  return frames[i];
}

/*!
\brief Get the set of frames.
*/
inline QVector<FrameScaled> MayaGeometrySet::GetFrames() const
{
  return frames;
}



/*!
\brief Get the number of frames in the instance set.
*/
inline int MayaGeometrySet::count() const
{
  return frames.count();
}



/*!
\brief Get the name of the geometry set.
*/
inline QString MayaGeometrySet::GetName() const
{
  return name;
}


class MayaResources;
class MayaInstanceSet;
class MayaInstanceAll;

class Camera;

// Set of geometry instances along with their frames
class MayaGeometryAll {
protected:
  QMap<QString, MayaGeometrySet> instances; //!< Map between the name of the geometric instances and the MayaGeometrySet structure. A map is used instead of an array to speed up instance search given an input name.
public:
  //! Empty 
  MayaGeometryAll() {}
  MayaGeometryAll(const MayaGeometry&);
        
  //! Empty
  ~MayaGeometryAll() {}

  // Setters
  void Append(const MayaGeometry&);
    
          
  

      
  
  
  
          
  
    
    
  bool Save_OBJ(const QString&, bool = true) const;
  
    
        
      
  

  
    
  
  // Friends
  friend class MayaGpuAll;
};






