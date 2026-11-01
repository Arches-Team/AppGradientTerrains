// Maya

#include "maya.h"

/*!
\class MayaGpu maya.h
\brief This class implements a mesh whose structure is optimized for the GPU.

\ingroup MayaGpuGroup
*/

/*!
\brief Create a Maya Gpu instance.
*/
MayaGpu::MayaGpu()
{
  boxRenderer = nullptr;
}

/*!
\brief Destry a Maya Gpu instance.
*/
MayaGpu::~MayaGpu()
{
  delete boxRenderer;
}

/*!
\brief Creates an instance.
\param mg Geometry.
*/
MayaGpu::MayaGpu(const MayaGeometry& mg)
{
  // Initialize parameters
  MayaGpu::name = mg.GetName();
  MayaGpu::nv = mg.size_vertex();
  MayaGpu::nt = mg.size_triangles() * 3;
  MayaGpu::bbox = mg.GetBox();

  MayaGpu::buffer_vertex = 0;
  MayaGpu::buffer_normal = 0;
  MayaGpu::buffer_color = 0;
  MayaGpu::buffer_UVmap = 0;
  MayaGpu::buffer_index = 0;
  MayaGpu::buffer_texture = 0;
  MayaGpu::buffer_material = 0;

  MayaGpu::mat = mg.GetMaterial();

  MayaGpu::boxRenderer = nullptr;

  // Statistics from argument geometry
  statistics = mg.GetStatistics();
  geometryKey = mg.GetDataKey();

  // Create geometric and texture GPU data
  CreateGpu(mg);
  InitMaterial();
  InitTexture();
}

/*!
\brief Delete all buffers.
*/
void MayaGpu::DeleteBuffers()
{
  // Names are reset so that a second call cannot delete objects that reused them
  if (buffer_vertex != 0)   { glDeleteBuffers(1, &buffer_vertex); buffer_vertex = 0; }
  if (buffer_normal != 0)   { glDeleteBuffers(1, &buffer_normal); buffer_normal = 0; }
  if (buffer_index != 0)    { glDeleteBuffers(1, &buffer_index); buffer_index = 0; }
  if (buffer_color != 0)    { glDeleteBuffers(1, &buffer_color); buffer_color = 0; }
  if (buffer_UVmap != 0)    { glDeleteBuffers(1, &buffer_UVmap); buffer_UVmap = 0; }
  if (buffer_material != 0) { glDeleteBuffers(1, &buffer_material); buffer_material = 0; }
  if (buffer_texture != 0)  { glDeleteTextures(1, &buffer_texture); buffer_texture = 0; }

  if (boxRenderer != nullptr)
  {
    boxRenderer->DeleteBuffers();
    delete boxRenderer;
    boxRenderer = nullptr;
  }
}

/*!
\brief Create a MayaGpu from the MayaGeometry.
\param mg Geometry.
*/
void MayaGpu::CreateGpu(const MayaGeometry& mg)
{
  int cpt;
  int nv = mg.size_triangles() * 3;

  GLfloat* array_vertex = nullptr;
  GLfloat* array_normal = nullptr;
  GLfloat* array_color = nullptr;
  GLfloat* array_UVMap = nullptr;

  switch (mat.materialNode)
  {
  case VertexColor: // Use of Vertex Color
    array_color = new GLfloat[nv * 3];

    cpt = 0;
    for (int i = 0; i < nv; i++)
    {
      array_color[cpt * 3 + 0] = mg.GetColor(i / 3, i % 3)[0];
      array_color[cpt * 3 + 1] = mg.GetColor(i / 3, i % 3)[1];
      array_color[cpt * 3 + 2] = mg.GetColor(i / 3, i % 3)[2];

      cpt++;
    }
    // No break on vertexColor
  case None: // 
    array_vertex = new GLfloat[nv * 3];
    array_normal = new GLfloat[nv * 3];

    cpt = 0;
    for (int i = 0; i < nv; i++)
    {
      array_vertex[cpt * 3 + 0] = mg.GetVertex(0, i)[0];
      array_vertex[cpt * 3 + 1] = mg.GetVertex(0, i)[1];
      array_vertex[cpt * 3 + 2] = mg.GetVertex(0, i)[2];

      array_normal[cpt * 3 + 0] = mg.GetNormal(0, i)[0];
      array_normal[cpt * 3 + 1] = mg.GetNormal(0, i)[1];
      array_normal[cpt * 3 + 2] = mg.GetNormal(0, i)[2];

      cpt++;
    }
    break;
  case UVMapping: // Use of UVs Maps
    array_vertex = new GLfloat[nv * 3];
    array_normal = new GLfloat[nv * 3];
    array_UVMap = new GLfloat[nv * 2];

    cpt = 0;
    for (int i = 0; i < nv; i++)// Attention : il y a 3 �l�ments ici 
    {
      array_vertex[cpt * 3 + 0] = mg.GetVertex(0, i)[0];
      array_vertex[cpt * 3 + 1] = mg.GetVertex(0, i)[1];
      array_vertex[cpt * 3 + 2] = mg.GetVertex(0, i)[2];

      array_UVMap[cpt * 2 + 0] = mg.GetUV(0, i)[0];
      array_UVMap[cpt * 2 + 1] = mg.GetUV(0, i)[1];

      array_normal[cpt * 3 + 0] = mg.GetNormal(0, i)[0];
      array_normal[cpt * 3 + 1] = mg.GetNormal(0, i)[1];
      array_normal[cpt * 3 + 2] = mg.GetNormal(0, i)[2];

      cpt++;
    }
    break;
  }

  int* array_index = new int[nv];
  for (int i = 0; i < nv; i++)
  {
    array_index[i] = i;
  }

  // Generate And Bind The Vertex Buffer
  glGenBuffers(1, &buffer_vertex);						// Get A Valid Name
  glBindBuffer(GL_ARRAY_BUFFER, buffer_vertex);			// Bind The Buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * nv, array_vertex, GL_STATIC_DRAW);

  glGenBuffers(1, &buffer_normal);						// Get A Valid Name
  glBindBuffer(GL_ARRAY_BUFFER, buffer_normal);			// Bind The Buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * nv, array_normal, GL_STATIC_DRAW);

  switch (mat.materialNode)
  {
  case VertexColor: // Use of Vertex Color
    glGenBuffers(1, &buffer_color);					// Get A Valid Name
    glBindBuffer(GL_ARRAY_BUFFER, buffer_color);		// Bind The Buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * nv, array_color, GL_STATIC_DRAW);
    break;
  case UVMapping: // Use of UVs Maps
    glGenBuffers(1, &buffer_UVmap);					// Get A Valid Name
    glBindBuffer(GL_ARRAY_BUFFER, buffer_UVmap);		// Bind The Buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 2 * nv, array_UVMap, GL_STATIC_DRAW);
    break;
  }

  // Generate And Bind The Index
  glGenBuffers(1, &buffer_index);							    // Get A Valid Name
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer_index);	// Bind The Buffer
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * nv, array_index, GL_STATIC_DRAW);

  // MayaGpu miv;
  Box b = mg.GetBox();
  /*switch (mat.materialNode){
  case None: miv = MayaGpu(name, nv, indexes.size(), buffer_vertex, buffer_normal, 0, buffer_index, mat, b);	break;
  case VertexColor: miv = MayaGpu(name, nv, indexes.size(), buffer_vertex, buffer_normal, buffer_color, buffer_index, mat, b);	break;
  case UVMapping: miv = MayaGpu(name, nv, indexes.size(), buffer_vertex, buffer_normal, buffer_UVmap, buffer_index, mat, b);	break;
  }*/

  delete[]array_vertex;
  delete[]array_normal;
  delete[]array_index;
  delete[]array_color;
  delete[]array_UVMap;
}/*!
\brief initialize the renderer
  */
void MayaGpu::InitRenderer()
{
  QVector<Vector> points;

  // X : X+1 lines
  points.push_back(Vector(bbox[0][0], bbox[0][1], bbox[0][2]));
  points.push_back(Vector(bbox[1][0], bbox[0][1], bbox[0][2]));

  points.push_back(Vector(bbox[0][0], bbox[1][1], bbox[0][2]));
  points.push_back(Vector(bbox[1][0], bbox[1][1], bbox[0][2]));

  points.push_back(Vector(bbox[0][0], bbox[0][1], bbox[1][2]));
  points.push_back(Vector(bbox[1][0], bbox[0][1], bbox[1][2]));

  points.push_back(Vector(bbox[0][0], bbox[1][1], bbox[1][2]));
  points.push_back(Vector(bbox[1][0], bbox[1][1], bbox[1][2]));

  // Y : Y+1 lines
  points.push_back(Vector(bbox[0][0], bbox[0][1], bbox[0][2]));
  points.push_back(Vector(bbox[0][0], bbox[1][1], bbox[0][2]));

  points.push_back(Vector(bbox[1][0], bbox[0][1], bbox[0][2]));
  points.push_back(Vector(bbox[1][0], bbox[1][1], bbox[0][2]));

  points.push_back(Vector(bbox[0][0], bbox[0][1], bbox[1][2]));
  points.push_back(Vector(bbox[0][0], bbox[1][1], bbox[1][2]));

  points.push_back(Vector(bbox[1][0], bbox[0][1], bbox[1][2]));
  points.push_back(Vector(bbox[1][0], bbox[1][1], bbox[1][2]));

  // Z : Z+1 lines
  points.push_back(Vector(bbox[0][0], bbox[0][1], bbox[0][2]));
  points.push_back(Vector(bbox[0][0], bbox[0][1], bbox[1][2]));

  points.push_back(Vector(bbox[1][0], bbox[0][1], bbox[0][2]));
  points.push_back(Vector(bbox[1][0], bbox[0][1], bbox[1][2]));

  points.push_back(Vector(bbox[0][0], bbox[1][1], bbox[0][2]));
  points.push_back(Vector(bbox[0][0], bbox[1][1], bbox[1][2]));

  points.push_back(Vector(bbox[1][0], bbox[1][1], bbox[0][2]));
  points.push_back(Vector(bbox[1][0], bbox[1][1], bbox[1][2]));

  boxRenderer = new MayaSimpleRenderer(points, Color(0.1, 0.1, 0.1), GL_LINES);
}/*!
\brief Initialize the material of the object.
*/
void MayaGpu::InitMaterial()
{
  if (buffer_material == 0) glGenBuffers(1, &buffer_material);

  // Material buffer initialization
  material.ambient[0] = mat.ambient[0]; material.ambient[1] = mat.ambient[1]; material.ambient[2] = mat.ambient[2]; material.ambient[3] = mat.ambient[3];
  material.diffuse[0] = mat.diffuse[0]; material.diffuse[1] = mat.diffuse[1]; material.diffuse[2] = mat.diffuse[2]; material.diffuse[3] = mat.diffuse[3];
  material.specular[0] = mat.specular[0]; material.specular[1] = mat.specular[1]; material.specular[2] = mat.specular[2]; material.specular[3] = mat.specular[3];
  material.shininess = mat.shininess;

  glBindBuffer(GL_UNIFORM_BUFFER, buffer_material);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(material), &material, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, buffer_material); // Binding point 1 is the transform block
}


/*!
\brief Initialize the texture.
*/
void MayaGpu::InitTexture()
{
  if (mat.materialNode == UVMapping && !mat.texture.albedo.isNull())
  {
    if (buffer_texture == 0) glGenTextures(1, &buffer_texture);					// Create The Texture

    QImage albedo = mat.texture.albedo.convertToFormat(QImage::Format::Format_RGBA8888);
    if (!mat.texture.opacity.isNull())
      albedo.setAlphaChannel(mat.texture.opacity);

    int width = albedo.width();
    int height = albedo.height();

    // Typical Texture Generation Using Data From The Bitmap
    // Unit 0 is free, units 1 to 14 hold the textures of MayaWidget and must stay bound
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, buffer_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    const QImage flippedAlbedo = albedo.flipped();
#else
    const QImage flippedAlbedo = albedo.mirrored(false, true);   // QImage::flipped() only exists since Qt 6.9
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, flippedAlbedo.bits());

    glBindTexture(GL_TEXTURE_2D, 0);
  }
}