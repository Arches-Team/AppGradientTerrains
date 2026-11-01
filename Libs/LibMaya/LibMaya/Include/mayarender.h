#pragma once

#include <QtCore/QtCore>
#include <QtGui/QMatrix4x4>
#include "camera.h"
#include "segment.h"
#include "mayashader.h"
#include "color.h"

#include "evectorfloat.h"

class QOpenGLContextGroup;

class MayaSimpleRenderer
{
protected:
  GLuint program = 0; //!< Program used to draw line, shared by all the renderers of a group of contexts.
  QOpenGLContextGroup* programGroup = nullptr; //!< Group of contexts of the program.
  GLuint VBO = 0; //!< Where we store our vertex.
  GLuint VAO = 0; //!< VAO.
  Color color = Color::White; //!< Color of our lines.
  VectorFloat* map = nullptr; //!< CPU map of our VBO.

  // The program is shared, so the matrices of each renderer are kept here and set at each draw
  QMatrix4x4 view; //!< View matrix.
  QMatrix4x4 projection; //!< Projection matrix.
  QMatrix4x4 model; //!< Model-to-world matrix.

  GLuint initSizeLines; //!< Initial size of our lines vector.
  GLuint currentSizeLines; //!< How many vertex there is in our VBO.

  GLenum lineType; //!< type of line to draw: lines or line loops.

  bool depthTest = true;
  float lineWidth = 1.0; //!< Width.
public:
    MayaSimpleRenderer(const QVector<Vector>&, const Color&, GLenum);
  MayaSimpleRenderer(Segment*, int, const Color&, GLenum);
  MayaSimpleRenderer(Vector*, int, const Color&, GLenum);
  ~MayaSimpleRenderer();

  // The renderer owns its buffers and a reference to its program, a copy would release them twice
  MayaSimpleRenderer(const MayaSimpleRenderer&) = delete;
  MayaSimpleRenderer& operator=(const MayaSimpleRenderer&) = delete;

  void DeleteBuffers();
    void Draw();
  void SetCamera(const Camera& c, int width, int height);
  void SetModelMatrix(const QMatrix4x4& model);

  void setDepthTest(bool d) { depthTest = d; }
  void setLineWidth(float l) { lineWidth = l; }
protected:
  void AcquireProgram();
  void ReleaseProgram();
};

