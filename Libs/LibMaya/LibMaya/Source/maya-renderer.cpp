#include "mayarender.h"

#include "cpu.h"
#include <QtGui/QMatrix4x4>
#include <QtGui/QOpenGLContext>
/*!
\brief Build a simple renderer.
\param lines All points on the line(s).
\param color Color of the lines.
\param lineType Type of line to draw.
*/
MayaSimpleRenderer::MayaSimpleRenderer(const QVector<Vector>& lines, const Color& color, GLenum lineType) : lineType(lineType), color(color)
{
  AcquireProgram();

  initSizeLines = lines.size();
  currentSizeLines = lines.size();

  //Build the VBO and map it
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferStorage(GL_ARRAY_BUFFER, lines.size() * sizeof(VectorFloat), nullptr, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
  map = (VectorFloat*)glMapBufferRange(GL_ARRAY_BUFFER, 0, lines.size() * sizeof(VectorFloat), GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);

  //Upload data and signal OpenGL
  for (int i = 0; i < lines.size(); i++)
  {
    map[i] = VectorFloat(lines[i]);
  }

  glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);

  //Create the VAO and enable the position attrib.
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VectorFloat), (void*)offsetof(VectorFloat, x));
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

}

/*!
\brief Build a simple renderer.
\param lines All points on the line(s).
\param n Number of points.
\param color Color of the lines.
\param lineType Type of line to draw.
*/
MayaSimpleRenderer::MayaSimpleRenderer(Vector* lines, int n, const Color& color, GLenum lineType) : lineType(lineType), color(color)
{
  AcquireProgram();

  initSizeLines = n;
  currentSizeLines = n;

  //Build the VBO and map it
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferStorage(GL_ARRAY_BUFFER, n * sizeof(VectorFloat), nullptr, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
  map = (VectorFloat*)glMapBufferRange(GL_ARRAY_BUFFER, 0, n * sizeof(VectorFloat), GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);

  //Upload data and signal OpenGL
  for (int i = 0; i < n; i++)
  {
    map[i] = VectorFloat(lines[i]);
  }

  glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);

  //Create the VAO and enable the position attrib.
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VectorFloat), (void*)offsetof(VectorFloat, x));
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

}
/*!
\brief Build a simple renderer.
\param lines All points on the line(s).
\param n Number of points.
\param color Color of the lines.
\param lineType Type of line to draw.
*/
MayaSimpleRenderer::MayaSimpleRenderer(Segment* lines, int n, const Color& color, GLenum lineType) : lineType(lineType), color(color)
{
  AcquireProgram();

  initSizeLines = 2 * n;
  currentSizeLines = 2 * n;

  //Build the VBO and map it
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferStorage(GL_ARRAY_BUFFER, 2 * n * sizeof(VectorFloat), nullptr, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
  map = (VectorFloat*)glMapBufferRange(GL_ARRAY_BUFFER, 0, 2 * n * sizeof(VectorFloat), GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);

  //Upload data and signal OpenGL
  for (int i = 0; i < n; i++)
  {
    map[2 * i] = VectorFloat(lines[i].Vertex(0));
    map[2 * i + 1] = VectorFloat(lines[i].Vertex(1));
  }

  glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);

  //Create the VAO and enable the position attrib.
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VectorFloat), (void*)offsetof(VectorFloat, x));
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

}/*!
\brief Set the camera.
\param c Camera to set.
*/
void MayaSimpleRenderer::SetCamera(const Camera& c, int width, int height)
{
  // Set the camera
  view = QMatrix4x4();
  view.lookAt(QVector3D(c.Eye()[0], c.Eye()[1], c.Eye()[2]), QVector3D(c.At()[0], c.At()[1], c.At()[2]), QVector3D(c.Up()[0], c.Up()[1], c.Up()[2]));

  projection = QMatrix4x4();
  projection.perspective(
    qRadiansToDegrees(c.GetAngleOfViewV(width, height)), // Qt attend des degr�s
    static_cast<float>(width) / static_cast<float>(height),
    c.GetNear(),
    c.GetFar()
  );

  // Setting the camera resets the model matrix
  model = QMatrix4x4();
}

/*!
\brief Set the per-instance model matrix.
\param m Model-to-world transform.
*/
void MayaSimpleRenderer::SetModelMatrix(const QMatrix4x4& m)
{
  model = m;
}

/*!
\brief Draw the renderer.
*/
void MayaSimpleRenderer::Draw()
{
  if (depthTest)
  {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
  }
  else
    glDisable(GL_DEPTH_TEST);

  glLineWidth(lineWidth);

  //Bind program and VAO
  glUseProgram(program);
  glBindVertexArray(VAO);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VectorFloat), (void*)offsetof(VectorFloat, x));

  //Update matrices and color, the program is shared with other renderers, and draw
  glUniformMatrix4fv(0, 1, GL_FALSE, view.constData());
  glUniformMatrix4fv(1, 1, GL_FALSE, projection.constData());
  glUniformMatrix4fv(3, 1, GL_FALSE, model.constData());
  glUniform4f(2, color[0], color[1], color[2], color[3]);

  glDrawArrays(lineType, 0, currentSizeLines);
  glBindVertexArray(0);
  glUseProgram(0);
  glDisable(GL_DEPTH_TEST);
}

/*!
\brief Delete all buffers and release the shared program.
*/
void MayaSimpleRenderer::DeleteBuffers()
{
  if (VBO != 0) {
    // Deleting the buffer also unmaps it
    glDeleteBuffers(1, &VBO); VBO = 0; map = nullptr;
  }
  if (VAO != 0) {
    glDeleteVertexArrays(1, &VAO); VAO = 0;
  }
  ReleaseProgram();
}

/*!
\brief Release the buffers and the shared program.

The OpenGL context in which the renderer was created must be current.
*/
MayaSimpleRenderer::~MayaSimpleRenderer()
{
  DeleteBuffers();
}

namespace
{
  //! Program shared by the simple renderers of a group of contexts, with the number of renderers using it.
  struct SharedSimpleProgram
  {
    GLuint program = 0;
    int users = 0;
  };

  //! Shared programs, by group of contexts: a program can only be used in the contexts of its group.
  QHash<QOpenGLContextGroup*, SharedSimpleProgram>& SharedSimplePrograms()
  {
    static QHash<QOpenGLContextGroup*, SharedSimpleProgram> programs;
    return programs;
  }
}

/*!
\brief Get the program shared by the renderers of the group of the current context, and compile it for the first one.
*/
void MayaSimpleRenderer::AcquireProgram()
{
  QOpenGLContext* context = QOpenGLContext::currentContext();
  programGroup = context != nullptr ? context->shareGroup() : nullptr;

  SharedSimpleProgram& shared = SharedSimplePrograms()[programGroup];
  if (shared.program == 0)
  {
    MayaShader shader(System::GetResource("ARCHESLIBDIR").append("/LibMaya/Shaders/simple"));
    shared.program = shader.Detach();

    // The program disappears with the contexts of the group, which may then be reused at the same address
    if (programGroup != nullptr)
    {
      QOpenGLContextGroup* group = programGroup;
      QObject::connect(group, &QObject::destroyed, [group]() { SharedSimplePrograms().remove(group); });
    }
  }
  shared.users++;
  program = shared.program;
}

/*!
\brief Release the shared program, which is deleted with its last renderer.
*/
void MayaSimpleRenderer::ReleaseProgram()
{
  if (program == 0) return;

  QHash<QOpenGLContextGroup*, SharedSimpleProgram>& programs = SharedSimplePrograms();
  QHash<QOpenGLContextGroup*, SharedSimpleProgram>::iterator shared = programs.find(programGroup);
  if (shared != programs.end() && shared->program == program && --shared->users == 0)
  {
    glDeleteProgram(program);
    programs.erase(shared);
  }
  program = 0;
}
