#include "qte.h"
#include "camera.h"

/*!
\class MayaWidgetTerrainEditor qte.h
\brief This class implements a mayawidget editor.
*/

/*!
\brief Empty.
*/
MayaWidgetTerrainEditor::MayaWidgetTerrainEditor()
{
  tool = 1;
  showAncre(true);
  //material = 0;
  _height = 0.0;
  _radius = 5.;
  SetAnchor(Vector(0, 0, 0));
  Nx = 0;
  Ny = 0;

  cloneok = false;
  lassoOk = false;
  newlassoDraw = false;
  lineOk = false;
  newLineDraw = false;
  newDestLineDraw = false;
  blendThreshold = 1;
  directionok = false;

  beginstroke = false;
  modified = false;
  editable = false;
  usePlane = false;
  useAncre = false;
}

/*!
\brief Release the GPU resources while the OpenGL context is still valid.
*/
MayaWidgetTerrainEditor::~MayaWidgetTerrainEditor()
{
  makeCurrent();
  tools.Release();
  direction.DeleteBuffers();
  doneCurrent();
}

/*!
\brief Render the scene.

The commands for drawing the scene are performed here.
*/
void MayaWidgetTerrainEditor::paintGL()
{
  QElapsedTimer t;
  t.start();

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  gluLookAt(camera.Eye()[0], camera.Eye()[1], camera.Eye()[2], camera.At()[0], camera.At()[1], camera.At()[2], camera.Up()[0], camera.Up()[1], camera.Up()[2]);

  // NVIDIA Graphic Card & Opengl 4 Supported
  if (strcmp((char*)glGetString(GL_VENDOR), "NVIDIA Corporation") == 0 && strncmp((char*)glGetString(GL_VERSION), "4.2.0", 2) == 0)
  {
    // Rendu avec instances (Opengl 4.0)
    updateGPU();

    if (useBackG) RenderBackGround();
    if (useLight) RenderLight();

    //if (cloneok) source.Render(gpuparam);

    if (useWorld) gpuall.Render(gpuparam);
    if (usePlane) plane.Render(camera, width(), height());
    RenderForeGround(t);

    tools.Render(camera, width(), height());

    if (useAncre) RenderAnchor();
    if (directionok) direction.Render(gpuparam);
  }
}

/*!
\brief Update the position of the Anchor on the intersection between the ray (mouse direction and the plane).
\param e Mouse event.
*/
void MayaWidgetTerrainEditor::UpdateAnchor(QMouseEvent* e)
{
  if (!hasFocus()) setFocus(Qt::MouseFocusReason);

  Ray ray = ConvertPixelToRay(e->pos());

  // Intersection with HeightField
  double t;
  Vector intersectionPoint;
  HeightField hf = displayedHF;

  if (hf.Intersect(ray, t, intersectionPoint, hf.GetBox(), hf.K()))
  {
    SetAnchor(intersectionPoint);
  }
  else if (plane.Intersect(ray, t)) {
    SetAnchor(ray(t));
  }

  updateToolAnchor();
}

/*!
\brief Update Tool Anchor.
*/
void MayaWidgetTerrainEditor::updateToolAnchor()
{
  tools.setCircle(false);
  tools.setCircleCpy(false);
  tools.setBorder(false);
  tools.setLine(false);
  tools.setLineCpy(false);
  tools.setLasso(false);
  tools.setLassoCpy(false);

  if (tool == 1 || tool == 2 || tool == 13 || tool == 16 || tool == 14 || tool == 15)
  {
    // Cercle autour du pointeur
    circlePoints.clear();
    Circle2 circle(ancre, tool_rayon);
    for (int i = 0; i < 100; i++)
    {
      Vector2 pos = circle.Vertex(Math::Angle(i, 100));
      circlePoints.append(displayedHF.Vertex(pos));
    }

    tools.DataCircle(circlePoints);
    tools.setCircle(true);
  }
  else if (tool == 6 || tool == 9)
  {
    if (newlassoDraw || lassoOk)
      tools.setLasso(true);

    if (lassoPoints.size() > 0 && beginstroke == false && tool == 6)
    {
      // Clone Zone
      QVector<Vector> tmpZone = lassoPoints;

      QPoint centerPoint = lassoPolygon.boundingRect().center();
      Vector center = displayedHF.Vertex(Vector2(centerPoint.x(), centerPoint.y()));

      for (int i = 0; i < tmpZone.size(); i++)
      {
        tmpZone[i] += ancre - center;
      }
      tools.DataLassoCopy(tmpZone);
      tools.setLassoCpy(true);
    }
  }
  else if (tool == 5 || tool == 7)
  {
    if (newLineDraw || lineOk)
      tools.setLine(true);

    if (lineDestPoints2.size() > 0)
    {
      // Clone Zone
      QVector<Vector> tmpLin;

      // Calcul baricentre
      for (int i = 0; i < lineDestPoints2.size(); i++)
        tmpLin.append(displayedHF.Vertex(lineDestPoints2[i]));

      tools.DataLineCopy(tmpLin);

      tools.setLine(true);
      tools.setLineCpy(true);
    }

    if (clicDragPoints.size() > 0)
    {
      // Clone Zone
      QVector<Vector> tmpLin;

      // Calcul baricentre
      for (int i = 0; i < clicDragPoints.size(); i++)
        tmpLin.append(displayedHF.Vertex(clicDragPoints[i]));

      tools.DataLineCopy(tmpLin);

      tools.setLineCpy(true);
    }

    if (tool == 5)
    {

      // Cercle autour du pointeur
      circlePoints.clear();
      Circle2 circle(ancre, tool_rayon);
      for (int i = 0; i < 100; i++)
      {
        Vector2 pos = circle.Vertex(Math::Angle(i, 100));
        circlePoints.append(displayedHF.Vertex(pos));
      }

      tools.DataCircle(circlePoints);
      tools.setCircle(true);

      // Cercle de copie
      if (cloneok)
      {
        circlePoints.clear();
        circle = Circle2(clonesource - (clicDragPoints.size() > 0 ? strokebegin - ancre : Vector(0.)), tool_rayon);
        for (int i = 0; i < 100; i++)
        {
          Vector2 pos = circle.Vertex(Math::Angle(i, 100));
          circlePoints.append(displayedHF.Vertex(pos));
        }

        tools.DataCircleCopy(circlePoints);
        tools.setCircleCpy(true);

        // Clone Zone
        if (clicDragPoints.size() > 0)
        {
          QVector<Vector> tmpLin;

          // Calcul baricentre
          for (int i = 0; i < clicDragPoints.size(); i++)
            tmpLin.append(displayedHF.Vertex(clicDragPoints[i] - (strokebegin - clonesource)));

          tools.DataLine(tmpLin);
          tools.setLine(true);
        }
      }
    }
  }
  else if (tool == 17)
  {
    QVector<Vector> circleTmp;
    QVector<Vector> borderTmp;
    // Cercle autour du pointeur
    circlePoints.clear();

    Circle2 circle(ancre, tool_rayon);
    for (int i = 0; i < 100; i++)
    {
      Vector2 pos = circle.Vertex(Math::Angle(i, 100));
      circlePoints.append(displayedHF.Vertex(pos));
      circleTmp.append(pos.ToVector(tool_height * HFMAGN));

      if (i % 10 == 0)
      {
        borderTmp.append(circlePoints[i]);
        borderTmp.append(circleTmp[i]);
      }
    }

    tools.DataCircle(circlePoints);
    tools.DataCircleCopy(circleTmp);
    tools.DataBorder(borderTmp);

    tools.setCircle(true);
    tools.setCircleCpy(true);
    tools.setBorder(true);
  }
  else if (tool == 18)
  {

    // Cercle autour du pointeur
    circlePoints.clear();
    Circle2 circle(ancre, tool_rayon);
    for (int i = 0; i < 100; i++)
    {
      Vector2 pos = circle.Vertex(Math::Angle(i, 100));
      circlePoints.append(displayedHF.Vertex(pos));
    }

    tools.DataCircle(circlePoints);
    tools.setCircle(true);
    if (clicDragPoints.size() > 0)
    {
      // Clone Zone
      QVector<Vector> borderTmp;
      QVector<Vector> tmpLin;
      QVector<Vector> tmpLin2;

      // Calcul baricentre
      for (int i = 0; i < clicDragPoints.size(); i++)
      {
        Vector pos = displayedHF.Vertex(clicDragPoints[i]);
        tmpLin.append(pos);
        Vector pos2 = pos;
        pos2[2] = tool_height * HFMAGN;
        tmpLin2.append(pos2);
        if (i % 10 == 0 || i == clicDragPoints.size() - 1)
        {
          borderTmp.append(pos);
          borderTmp.append(pos2);
        }
      }

      tools.DataLine(tmpLin2);
      tools.DataLineCopy(tmpLin);
      tools.DataBorder(borderTmp);

      tools.setLine(true);
      tools.setLineCpy(true);
      tools.setBorder(true);
    }
  }
  else if (tool == 21 || tool == 22 || tool == 20 || tool == 19 || tool == 23)
  {
    // Cercle autour du pointeur
    circlePoints.clear();
    Circle2 circle(ancre, tool_rayon);
    for (int i = 0; i < 100; i++)
    {
      Vector2 pos = circle.Vertex(Math::Angle(i, 100));
      circlePoints.append(displayedHF.Vertex(pos));
    }

    tools.DataCircle(circlePoints);
    tools.setCircle(true);

    if (clicDragPoints.size() > 0)
    {
      // Clone Zone
      QVector<Vector> tmpLin;

      // Calcul baricentre
      for (int i = 0; i < clicDragPoints.size(); i++)
        tmpLin.append(displayedHF.Vertex(clicDragPoints[i]));

      tools.DataLineCopy(tmpLin);

      tools.setLineCpy(true);
    }
  }
}/*!
\brief Overloaded function to capture the mouse events.

This function implements controls.
\param e Mouse event.
*/
void MayaWidgetTerrainEditor::mouseReleaseEvent(QMouseEvent* e)
{
  if (!editable)
    return;

  setCursor(Qt::ArrowCursor);

  releasePoint = Vector2(e->pos().x(), e->pos().y());
  tools.setLineCpy(false);

  if (beginstroke)
  {
    beginstroke = false;

    if (newlassoDraw) {
      //if a new lasso area has been drawn
      lassoPolygon.append(lassoPolygon[0]); //close the polygon
      lassoOk = true;
      updateToolAnchor();
    }

    if (newLineDraw) {
      //if a new line has been drawn
      lineOk = true;
    }

    if (tool == 6) {

    }
    else if (tool == 7 && newDestLineDraw && lineDestPoints2.count() > 0 && linePoints3.count() > 0)
    {
      emit _Tool(Vector(0, 0, 0), tool_rayon);  //source vector not used for the clone broken line tool
      lineDestPoints2.clear();
      tools.setLineCpy(false);
      updateToolAnchor();
    }
    else if (tool == 9) {        // slopeOrientation tool needs clic and drag before emitting tool (for orientation direction)
      direction.ClearFrames();
      if (lassoPolygon.size() > 2 && directionok) //if there is a polygon and we clic and dragged in a direction 
      {
        emit _Tool(strokebegin, tool_rayon);
        directionok = false;
      }
      else
      {
        std::cout << "You must first draw a line to clone with CTRL+leftClick " << std::endl;
      }
    }
    else {
      if (!clicDragPoints.isEmpty()) {
        emit _Tool(Vector(0, 0, 0), tool_rayon);
        clicDragPoints.clear();

        tools.setLine(false);

        tools.setBorder(false);
        updateToolAnchor();
      }
    }
  }
  if (modified) {
    emit _AutoReconstruct();
  }
  modified = false;

  MayaWidget::mouseReleaseEvent(e);
}


/*!
\brief Binds the mouse buttons to actions
tools clone zone (tool 6), cratere effect (tool 10) and river effect (tool 11) are applied on mouse click, all the others are called on mouse release
*/
void MayaWidgetTerrainEditor::mousePressEvent(QMouseEvent* e)
{
  setCursor(Qt::CrossCursor);

  x0 = e->globalPosition().x();
  y0 = e->globalPosition().y();

  std::cout << "Click" << std::endl;
  newlassoDraw = false;
  newLineDraw = false;
  newDestLineDraw = false;

  if (!editable)
    return;

  Ray ray = ConvertPixelToRay(e->pos());
  double t;
  Vector intersectionPoint;
  HeightField hf = displayedHF;

  if (hf.Intersect(ray, t, intersectionPoint, hf.GetBox(), hf.K()))
  {
    if (e->button() == Qt::LeftButton && !(e->modifiers() & Qt::ControlModifier) && !(e->modifiers() & Qt::ShiftModifier) && !(e->modifiers() & Qt::AltModifier)) // left button without any modifier -> apply a tool (with stroke or only click)
    {
      modified = true;

      if (tool == 6 || tool == 7 || tool == 8 || tool == 9 || tool == 10 || tool == 11 || tool == 12) { // not base Stroke tools so we don't change the anchor material to red like for a stroke
        if (tool == 6 || tool == 8) {    //click and drag for stroke not available with the zone clone tool
          if (lassoPolygon.size() >= 3) {
            std::cout << "cloning the selected zone at the selected location" << std::endl;
          }
          else {
            std::cout << "You first need to select a zone by holding SHIFT and left click and draging around the desired zone's borders" << std::endl;
          }
        }
        if (tool == 10) { //cratere effect
          if (lassoPolygon.size() >= 3) {
            std::cout << "applying cratere effect in the selected zone" << std::endl;
          }
          else {
            std::cout << "You first need to select a zone by holding SHIFT and left click and draging around the desired zone's borders" << std::endl;
            return;
          }
        }
      }
      else {
        std::cout << "Stroke" << std::endl;
        beginstroke = true;
      }


      strokebegin = intersectionPoint;
      if (tool == 5) {
        if (cloneok) {
          clicDragPoints.append(intersectionPoint);
        }
        else {
          std::cout << "first you need to place the cloning source on the terrain with a right clic" << std::endl;
        }
      }
      else if (tool == 6) { //click to clone the zone (not on release)
        emit _Tool(intersectionPoint, tool_rayon);
      }
      else if (tool == 7 || tool == 18 || tool == 19 || tool == 20) {  //we must emit tool on Mouse release for tool 7 (cloneBrokenLine) and not on Mouse click nor Mouse move
        std::cout << "New start Dest line" << std::endl;

        beginstroke = true; //stroke to draw the dest Line
        lineDestPoints2.clear();

        newDestLineDraw = true;
      }
      else if (tool == 9) {
        if (lassoPoints.size() >= 3) {
          beginstroke = true; //to clic and drag to get the orientation of the slopes

          std::cout << "Now drag the direction you want to oriente the slopes" << std::endl;
        }
      }
      else if (tool == 10) {  //click anywhere to apply the tool to the selected zone (only if a zone has been selected)
        if (lassoPolygon.size() >= 3) {
          emit _Tool(Vector(0, 0, 0), tool_rayon);
        }
      }
      else if (tool == 11) {//click anywhere to apply the tool
        emit _Tool(Vector(0, 0, 0), tool_rayon);
      }
      else if (tool == 12) {//click anywhere to apply the tool
        emit _Tool(Vector(0, 0, 0), tool_rayon);
      }
      else {
        clicDragPoints.append(intersectionPoint);
      }
    }
    else if (e->button() == Qt::LeftButton && (e->modifiers() & Qt::ControlModifier)) { // left button with CTRL

      tools.setLine(false);
      tools.setLineCpy(false);
      tools.setCircle(false);
      tools.setLasso(false);
      tools.setLassoCpy(false);

      if (tool == 5)
      {
        std::cout << "New clone source position" << std::endl;

        clonesource = intersectionPoint;
        cloneok = true;
      }
      else if (tool == 6 || tool == 8 || tool == 9)
      {
        std::cout << "New start lasso zone" << std::endl;
        lassoPolygon.clear();
        lassoPoints.clear();
        beginstroke = true;

        //add first point of lasso zone
        lassoPoints.push_back(intersectionPoint);
        lassoPolygon << (QPoint(intersectionPoint[0], intersectionPoint[1]));

        lassoOk = true;
        newlassoDraw = true;

        tools.setLasso(true);
      }
      else if (tool == 7)
      {
        std::cout << "New start line" << std::endl;
        linePoints2.clear();
        linePoints3.clear();
        beginstroke = true;

        //add first point of line

        linePoints3.append(intersectionPoint);
        linePoints2.append(Vector2(intersectionPoint[0], intersectionPoint[1]));

        lineOk = true;
        newLineDraw = true;

        tools.setLine(true);
      }
      else if (tool == 17 || tool == 18) // add constraint -> pick altitude
      {
        emit _PickAltitude(intersectionPoint[2], tool);
      }
    }
    else
    {
      MayaWidget::mousePressEvent(e);
    }
  }
}


/*!
\brief  Binds the mouse move to actions
\param e Events
*/
void MayaWidgetTerrainEditor::mouseMoveEvent(QMouseEvent* e)
{
  int x = e->globalPosition().x();
  int y = e->globalPosition().y();

  if (!editable)
    return;

  if (beginstroke)
  {
    Ray ray = ConvertPixelToRay(e->pos());
    double t;
    Vector intersectionPoint;
    HeightField hf = displayedHF;

    if (hf.Intersect(ray, t, intersectionPoint, hf.GetBox(), hf.K()))
    {
      if (!(e->modifiers() & Qt::ControlModifier) && !(e->modifiers() & Qt::ShiftModifier) && !(e->modifiers() & Qt::AltModifier))  //only clicked, no modifiers -> use tool
      {
        if (tool != 6 && tool != 7 && tool != 8 && tool != 9 && tool != 10 && tool != 11) { //does not apply the tool during drag, but on release only with those tools


          if (tool == 5) {    //source clone tool, we have to move the source with the mouse to show what we are cloning
            if (!cloneok) { //if no clone source has been set on the map
              return;
            }
            Vector2 movedSource2d(GetCloneSource() + (intersectionPoint - strokebegin));
            if (!hf.Inside(movedSource2d)) {  //if the source point to clone is outside of the terrain
              return;
            }
          }

          std::cout << "Stroke" << std::endl;
          clicDragPoints.append(intersectionPoint);

        }
        else if (tool == 7 || tool == 18 || tool == 19 || tool == 20 || tool == 21 || tool == 22) {

          lineDestPoints2.append(Vector2(intersectionPoint[0], intersectionPoint[1]));
          updateToolAnchor();
        }
        else if (tool == 9) {

          QPoint center = lassoPolygon.boundingRect().center();

          Vector arrowStart = Vector(center.x(), center.y(), gradientTerrain.HeightAt(center.x(), center.y()));
          Vector arrowEnd = arrowStart + (intersectionPoint - strokebegin);
          arrowEnd[2] = gradientTerrain.HeightAt(arrowEnd[0], arrowEnd[1]);
          MayaGeometry mg("Direction", Mesh::Arrow(arrowStart, arrowEnd, 5., 10., Norm(intersectionPoint - strokebegin) - 10., 10), MayaMaterial(ShaderNode::ShaderPhong, Color(0.2, 0.6, 0.2, 0.4), Color(0.2, 0.6, 0.2, 0.4)));

          MayaGeometrySet mgs(mg, FrameScaled(Vector(0, 0, 10 + tool_rayon / 2.0), Vector(1, 1, 1)));
          // GPU buffers must be created/deleted in this widget's context
          makeCurrent();
          direction.DeleteBuffers();
          direction = MayaGpuSet(mgs);
          doneCurrent();
          directionok = true;

        }
      }
      else if (e->modifiers() & Qt::ControlModifier) { // left button with CTRL -> lasso Zone

        if (tool == 6 || tool == 8 || tool == 9)
        {
          lassoPoints.push_back(intersectionPoint);
          lassoPolygon << (QPoint(intersectionPoint[0], intersectionPoint[1]));

          tools.DataLasso(lassoPoints);
        }
        else if (tool == 7)
        {
          linePoints3.append(intersectionPoint);
          linePoints2.append(Vector2(intersectionPoint[0], intersectionPoint[1]));

          tools.DataLine(linePoints3);
        }
      }
    }
  }

  MayaWidget::mouseMoveEvent(e);
}


/*!
\brief Binds the mouse double clicks to actions.
\param e Events.
*/
void MayaWidgetTerrainEditor::mouseDoubleClickEvent(QMouseEvent* e)
{
  if (!editable)
    return;

  if (!(e->modifiers() & Qt::ControlModifier))
  {
    const QPoint& pixel = e->pos();
    Ray ray = ConvertPixelToRay(pixel);

    Vector pos;
    double t;
    if (plane.Intersect(ray, t))
    {
      camera.SetAt(ray(t));
    }
  }
  update();
}

/*!
\brief Binds the mouse wheel to actions.

This function handles zooming and changing markers height.
\param e Events.
*/
void MayaWidgetTerrainEditor::wheelEvent(QWheelEvent* e)
{
  if (!editable)
    return;

  int numDegrees = e->angleDelta().y() / 8;
  int numSteps = numDegrees / 15;

  if (e->modifiers() & Qt::ShiftModifier)
  {
    if (numSteps > 0)
      tool_rayon *= 1.3;
    else if (numSteps < 0)
      tool_rayon /= 1.3;
    std::cout << "New radius " << tool_rayon << std::endl;

    updateToolAnchor();
  }
  else
  {
    // Zoom progressif en fonction de la distance au terrain.
    Vector direction = camera.View();
    double currentDist = Norm(direction);
    direction = Normalized(direction);

    double speed = 0.1 * currentDist;

    camera.SetEye(camera.Eye() + direction * speed * numSteps);
  }

  update();
}

/*!
\brief Binds the keyboard to actions.

\param e Key Events.
*/
void MayaWidgetTerrainEditor::keyPressEvent(QKeyEvent* e)
{
  qDebug() << "Key press";
  if ((e->modifiers() & Qt::ControlModifier) && (e->key() == Qt::Key_Z))
  {
    qDebug() << "CtrlZ press";
    emit _Undo();
  }
  if ((e->modifiers() & Qt::ControlModifier) && (e->key() == Qt::Key_C))
  {
    qDebug() << "CtrlC press";
    emit _ToggleConstraintsVisibility();
  }

  MayaWidget::keyPressEvent(e);
}
