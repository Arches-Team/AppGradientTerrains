// Maya

#include "tools-render.h"

/*!
\class ToolsRender tools-render.h
\brief Overlays (lasso, lines, circles, border) displayed by the tools in the MayaWidget.

GPU buffers are only created or deleted in Render() and Release(), which must be called with a current OpenGL context.
Data setters only store the data and mark the overlay as dirty, so they can be safely called from event handlers.

\ingroup MayaCore
*/

static const Color purpleColor(0.4, 0.2, 0.4);
static const Color greenColor(0.2, 0.6, 0.2);
static const Color greenLightColor(0.3, 0.8, 0.3);

/*!
\brief Create the tool overlays.
*/
ToolsRender::ToolsRender() :
  lasso(purpleColor, GL_LINE_LOOP, 5.),
  lassoCpy(greenColor, GL_LINE_LOOP, 5.),
  line(purpleColor, GL_LINE_STRIP, 5.),
  lineCpy(greenColor, GL_LINE_STRIP, 5.),
  circle(greenColor, GL_LINE_LOOP, 5.),
  circleCpy(purpleColor, GL_LINE_LOOP, 5.),
  border(greenLightColor, GL_LINES, 2.)
{
}

/*!
\brief Destructor.

GPU buffers should have been released with Release() while the OpenGL context was current;
only the CPU side objects are freed here.
*/
ToolsRender::~ToolsRender()
{
  for (Tool* t : { &lasso, &lassoCpy, &line, &lineCpy, &circle, &circleCpy, &border })
  {
    delete t->renderer;
    t->renderer = nullptr;
  }
}

/*!
\brief Release the GPU buffers of all overlays. Requires a current OpenGL context.
*/
void ToolsRender::Release()
{
  for (Tool* t : { &lasso, &lassoCpy, &line, &lineCpy, &circle, &circleCpy, &border })
    t->Release();
}

/*!
\brief Release the GPU buffers of the overlay. Requires a current OpenGL context.
*/
void ToolsRender::Tool::Release()
{
  if (renderer != nullptr)
  {
    renderer->DeleteBuffers();
    delete renderer;
    renderer = nullptr;
  }
}

/*!
\brief Render the overlay, rebuilding its renderer if the data changed. Requires a current OpenGL context.
*/
void ToolsRender::Tool::Render(const Camera& c, int width, int height)
{
  if (dirty)
  {
    Release();
    dirty = false;
  }

  if (!visible)
    return;

  if (renderer == nullptr && data.size() > 0)
  {
    renderer = new MayaSimpleRenderer(data, color, mode);
    renderer->setDepthTest(false);
    renderer->setLineWidth(lineWidth);
  }

  if (renderer != nullptr)
  {
    renderer->SetCamera(c, width, height);
    renderer->Draw();
  }
}

/*!
\brief Render the tool overlays.
*/
void ToolsRender::Render(const Camera& c, int width, int height)
{
  lasso.Render(c, width, height);
  lassoCpy.Render(c, width, height);
  border.Render(c, width, height);
  line.Render(c, width, height);
  lineCpy.Render(c, width, height);
  circle.Render(c, width, height);
  circleCpy.Render(c, width, height);
}
