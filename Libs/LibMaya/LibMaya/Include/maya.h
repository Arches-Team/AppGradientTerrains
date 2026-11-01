// Maya  

#pragma once

#ifdef _MSC_VER
#include "glew.h"
#else
#include "GL/glew.h"
#endif

#include <QtCore/QVector> 
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QApplication>
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QtGui/QPainter>

#include "mayarender.h"
#include "mayageometry.h" 


struct Transform {
  GLfloat ModelViewMatrix[16];			//!< modelview matrix of the transformation
  GLfloat ProjectionMatrix[16];			//!< projection matrix of the transformation
};

struct GpuUniform
{
  //Uniforms
  GLint loc_wireframe;
  GLint loc_texture;
  GLint loc_applyNormalMap;
  GLint loc_normalMapStrength;
  GLint loc_textureScaling;
  GLint loc_lightPos = -1; //!< Light position, updated at each frame.
  GLint loc_lightDirAxel = -1; //!< Light direction of the AxelTerrain model, updated at each frame.
  GLint loc_cameraPos = -1; //!< Camera position, updated at each frame.
  GLint loc_winScale = -1; //!< Half size of the window, updated at each frame.

  //Atributes
  GLint loc_position;
  GLint loc_normal;
  GLint loc_color;
  GLint loc_UV;
  GLint loc_inst_pos;
  GLint loc_inst_scl;
  GLint loc_inst_rot;

  //Subroutine indexes
  GLuint phongIndex;
  GLuint phongVCIndex;
  GLuint normalIndex;
  GLuint classiqueIndex;
  GLuint gridIndex;
  GLuint ignIndex;
  GLuint goochIndex;
  GLuint terrain3DIndex;
  GLuint UVTextureModel;
  GLuint bumpNormalIndex;
  GLuint ThinSnowIndex;
  GLuint GreyCoolWarmIndex;
  GLuint AxelTerrainIndex;
  GLuint WireframeIndex;
  GLuint WireframeAspectIndex;
  GLuint TriPlanarIndex;
};

struct GpuParameter
{
  MayaShader* shader_program = nullptr;
  GpuUniform uniforms;
  GLuint buffer_textureT[8] = {};	//!< Buffer of texture : Terrain(4) + AxelTerrain(4)

  // ENV
  GLuint buffer_textureE[6] = {};	//!< Buffer of texture : env

  Vector light; //!< Light
  Vector camera; //!< %Camera

  GLuint buffer_transform = 0;	//!< Uniform buffer transform
  Transform transform;		//!< Projection and modelview matrix 
};

struct MayaGpuMaterial
{
  GLfloat ambient[4];
  GLfloat diffuse[4];
  GLfloat specular[4];
  GLfloat shininess;
};





class MayaGpu;
class MayaGpuSet;
class MayaGpuAll;

class MayaResources;






















// Maya Gpu instance
class MayaGpu {
protected:
  QString name;              //!< Signature of the object, implemented as the name of the corresponding mesh file
  MayaStatistics statistics; //!< Statistics of the object, which are derived from the arguments of the constructor
  MayaMaterial mat;          //!< Material structure of the Object.

  GLuint buffer_vertex = 0; //!< Buffer of vertex
  GLuint buffer_normal = 0; //!< Buffer of normal
  GLuint buffer_color = 0;  //!< Buffer of color
  GLuint buffer_UVmap = 0;  //!< Buffer of UVMaps

  GLuint buffer_index = 0;  //!< Buffer of indexes

  MayaGpuMaterial material;	//!< Material data
  GLuint buffer_material = 0;	//!< Uniform material buffer
  GLuint buffer_texture = 0;    //!< Buffer of texture

  int nv = 0;               //!< Number of vertices
  int nt = 0;               //!< Number of triangles
  Box bbox;             //!< Bounding box

  MayaSimpleRenderer* boxRenderer; //!< Renderer used to draw the bounding box

  size_t geometryKey = 0; //!< Hash of the geometry sent to the GPU, see MayaGeometry::GetDataKey().

  void InitRenderer();
  void CreateGpu(const MayaGeometry&);
  void DeleteBuffers();

public:
  //! Empty
  MayaGpu();
  MayaGpu(const MayaGeometry&);
  ~MayaGpu();

    
    
  void InitMaterial();
  void InitTexture();

      
  };




// Set of Maya instances
class MayaGpuSet :public MayaGpu
{
protected:
  QVector<FrameScaled> frames;        //!< %Array of frames
  GLuint buffer_instances_position = 0; //!< Buffer of array of positions
  GLuint buffer_instances_rotation = 0; //!< Buffer of array of rotations
  GLuint buffer_instances_scale = 0;    //!< Buffer of array of scales
  int instanceCapacity = 0; //!< Number of instances the storage of the instance buffers can hold.

  GLuint buffer_Vao = 0;
public:
  //! Empty
  MayaGpuSet() {}
  MayaGpuSet(const MayaGeometrySet&);
  ~MayaGpuSet() {}

  void InitInstances();
  void InitVAO(GpuParameter&);

  void RefreshFrames();
  
  bool isWireframe() const;

  // Renderer
  void Render(GpuParameter&);
    void RenderBBox(Camera c, int width, int height);

  void SetSubroutine(GpuParameter&) const;

  // Setters
  void Append(const FrameScaled&);

  void ClearFrames();

      MayaStatistics GetStatistics() const;
public:
  void DeleteBuffers();
};

/*!
\brief Add an instance to the set.
\param frame The frame of the instance.
*/
inline void MayaGpuSet::Append(const FrameScaled& frame)
{
  // Statistics
  statistics.AddInstances();

  frames.append(frame);
}



// Set of instances along with their frames
class MayaGpuAll {
protected:
  QMap<QString, MayaGpuSet> instances; //!< Map of instances with their set of frames
public:
  //! Empty
  MayaGpuAll() {}
    //! Empty
  ~MayaGpuAll() {}

  // Renderer
  void Render(GpuParameter&);
    void RenderBBox(Camera camera, int width, int height);


  // Setters
  void Append(const MayaGeometrySet&);
  void Append(const MayaGeometryAll&);
  
      void Clear();
  void ClearFrames();

  
  // Statistics
  MayaStatistics GetStatistics() const;
};




class MayaPlane {
protected:
  Box2 area = Box2::Unit; //!< Area of the grid.
  double height = 0.0; //!< Elevation.
  double line = 1.0; //!< Line interval.
  MayaSimpleRenderer* planeRendererWhiteLine = nullptr;//!< Renderer used to draw the white line of the plane
  MayaSimpleRenderer* planeRendererQuad = nullptr;//!< Renderer used to draw the transparent quad of the plane
public:
  explicit MayaPlane(const Box2 & = Box2(10.0), const double& = 0.0, const double& = 1.0);
  ~MayaPlane();
  double Z() const;
        bool Intersect(const Ray&, double&) const;
  // Obsolete
  void Render(Camera c, int, int);
  void DeleteRenderers();
private:
  static const double epsilon; //!< Epsilon value for offseting lines when rendering the plane.
protected:
  void InitRenderer();
};

class MayaCameraSet
{
protected:
  QVector<Camera> cameras; //!< Set of cameras.
  int n; // Current camera index.
public:
  MayaCameraSet();
  void Push(const Camera&);
  void Pop();
    void Next(Camera&);
  };

class MayaWidget :public QOpenGLWidget
{
  Q_OBJECT
protected:
  MayaGpuAll gpuall;  //!< Instances for storing the objects on the GPU.
  MayaGpuSet anchor; //!< Internal instance for drawing the anchor point.
  MayaGpuSet lightshape; //!< Light shape.

  MayaGpuSet sphereshape; //!< Transparent sphere.

  Camera camera;//!< Current camera.

  int x0, y0; //!< Reference mouse coordinates.
  bool useAncre, usePlane, useWorld, useLight, useBBox, useBackG, useStats, useVAxis, useTime, useCamera; //!< Boolean flags.
  short int useCamRatio;
  MayaPlane plane = MayaPlane(Box2(Vector2(0.), Vector2(10.0)), 1.0); //!< Horizontal plane.
  Vector ancre = Vector::Null; //!< Focus point.
  Vector light; //!< Light point.
  GpuParameter gpuparam; //!< Set of parameters defining the way objects are rendered.
  MayaShader* gpubackground = nullptr; //!< Program used to draw background;
  GLuint backgroundVAO = 0; //!< VAO used when drawing the background.
  GLuint defaultVAO = 0; //!< VAO bound by default after initialization.

  MayaCameraSet cameras; //!< Set of cameras.
public:
  MayaWidget(QWidget* = nullptr);
  ~MayaWidget();

  // Camera
  void SetCamera(const Camera&);
  QVector<Camera> GetCameras() const;
  Camera GetCamera() const;
  void AddCamera();
  void DeleteCamera();
  void ChangeCamera();

  void SetPlanes(const double&, const double&);

  // Plane
  void SetPlane(const Box2&, const double&, const double&);

  // Lighting
  void SetLight(const Vector&);

  // Anchor
  void SetAnchor(const Vector&);
  Ray ConvertPixelToRay(const QPoint&) const;
  Ray ConvertPixelToRay(const QPoint&, int, int, int) const;

  // World Operators
  void SetWorld(const MayaGeometryAll&);
  void AppendWorld(const MayaGeometryAll&);
  void ReplaceInWorld(const MayaGeometryAll&);
  void ReplaceMaterialInWorld(MayaGeometryAll&);
  void ClearAll();

  void showAncre(bool b) { useAncre = b; }
  void showLight(bool b) { useLight = b; }
  void showPlane(bool b) { usePlane = b; }
  void showWorld(bool b) { useWorld = b; }
  void showBBox(bool b) { useBBox = b; }
  void showBackG(bool b) { useBackG = b; }
  void showStats(bool b) { useStats = b; }
  void showVAxis(bool b) { useVAxis = b; }
  void showTime(bool b) { useTime = b; }
  void showCamera(bool b) { useCamera = b; }
  void showCameraRatio(int b) { useCamRatio = b; }

  void SaveScreen(int = 1280, int = 720);
  void SaveScreen(int, int, const QString&);
protected:
  void initializeGL();
  void resizeGL(int, int);

  virtual void UpdateAnchor(QMouseEvent*);

  virtual void RenderAnchor();
  virtual void RenderLight();
  void RenderBackGround();
  void RenderForeGround(QElapsedTimer);

  virtual void InitGPU();
  void InitProgram();
  void InitCamera();

  virtual void InitAnchorShape();
  virtual void InitLightShape();
  virtual void InitSphereShape();
  void InitTerrainTextures();
  void InitEnvTextures();

  void InitLight();
  void updateGPU();
  void ReleaseGPU();

public:
  static bool Verbose;			//!< Static flag for verbose output in the console (useful for OpenGL debugging)
  static GLenum GLVerboseLevel;	//!< OpenGL verbosity level. Everything below is ignored.
  static void GLAPIENTRY OpenGLDebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* = nullptr);

public slots:
  void paintGL();

  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseReleaseEvent(QMouseEvent*);
  virtual void mouseDoubleClickEvent(QMouseEvent*);
  virtual void mouseMoveEvent(QMouseEvent*);
  virtual void wheelEvent(QWheelEvent*);
  virtual void keyPressEvent(QKeyEvent*);
  virtual void keyReleaseEvent(QKeyEvent*);

signals:
  void _signalEditHeight(const Vector&, int);
  void _signalEditSceneLeft(const Vector&);
  void _signalEditSceneRight(const Vector&);
  void _signalMouseMove();
};

