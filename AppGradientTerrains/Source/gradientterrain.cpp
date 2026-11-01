#include "gradientterrain.h"
#include "curvepoint.h"
#include "cubic.h"
#include "curveset.h"

#include <QtCore/QFile>
#include <QtCore/QDebug>

//using namespace std;

GradientTerrain::GradientTerrain() :VectorField2()
{}

GradientTerrain::GradientTerrain(const VectorField2& gradientSource) :VectorField2(gradientSource)
{
  Nx = GetSizeX() + 2;
  Ny = GetSizeY() + 2;

  Box2 b = gradientSource.GetBox();
  lap = ScalarField2(b, Nx - 2, Ny - 2, 0.);
  border = ScalarField2(b, Nx - 2, Ny - 2, 0.);
  tmp = ScalarField2(b, Nx - 2, Ny - 2, 0.);
  alpha = ScalarField2(b, Nx - 2, Ny - 2, 0.);
  alt = ScalarField2(b, Nx - 2, Ny - 2, 0.);
  alpha.Fill(1.0);
  UpdateHeightField();

}

GradientTerrain::GradientTerrain(const QImage& sourceImage, const bool& isHeightfield) :VectorField2(Box2(Vector2::Null, 400.0), sourceImage.width(), sourceImage.height())
{
  Box2 box(Vector2::Null, 400.0);

  /* grid dimensions */
  Nx = sourceImage.width() + 2;
  Ny = sourceImage.height() + 2;


  // omitting boundaries

  lap = ScalarField2(box, Nx - 2, Ny - 2, 0.);
  border = ScalarField2(box, Nx - 2, Ny - 2, 0.);
  tmp = ScalarField2(box, Nx - 2, Ny - 2, 0.);
  alpha = ScalarField2(box, Nx - 2, Ny - 2, 0.);
  alt = ScalarField2(box, Nx - 2, Ny - 2, 0.);
  alpha.Fill(1.0);

  if (!isHeightfield) {	//the image is a gradient image, with R and G composant being the x and y gradients value
    for (unsigned int i = 0; i < Nx - 2; i++) {
      for (unsigned int j = 0; j < Ny - 2; j++) {
        QColor color = sourceImage.pixelColor(i, j);
        (*this)(i, j) = Vector2((color.redF() - 0.5) * .2, (color.greenF() - 0.5) * .2);
      }
    }
  }
  else {	//the image is a heightfield image, with scales of grey, each pixel contains the heightfield value
    for (unsigned int i = 0; i < Nx - 2; i++) {
      for (unsigned int j = 0; j < Ny - 2; j++) {
        double xgrad, ygrad;
        if (i == 0) {
          xgrad = (sourceImage.pixelColor(i + 1, j).valueF() - sourceImage.pixelColor(i, j).valueF());
        }
        else if (i == Nx - 3) {
          xgrad = (sourceImage.pixelColor(i, j).valueF() - sourceImage.pixelColor(i - 1, j).valueF());
        }
        else {
          xgrad = (sourceImage.pixelColor(i + 1, j).valueF() - sourceImage.pixelColor(i - 1, j).valueF()) * 0.5;
        }

        if (j == 0) {
          ygrad = (sourceImage.pixelColor(i, j + 1).valueF() - sourceImage.pixelColor(i, j).valueF());
        }
        else if (j == Ny - 3) {
          ygrad = (sourceImage.pixelColor(i, j).valueF() - sourceImage.pixelColor(i, j - 1).valueF());
        }
        else {
          ygrad = (sourceImage.pixelColor(i, j + 1).valueF() - sourceImage.pixelColor(i, j - 1).valueF()) * 0.5;
        }
        (*this)(i, j) = 1. * Vector2(xgrad, ygrad);
      }
    }
  }

  UpdateHeightField();
}

GradientTerrain::GradientTerrain(const QString& sourceImagePath, const bool& isHeightfield) :GradientTerrain(QImage(sourceImagePath), isHeightfield)
{}


GradientTerrain::~GradientTerrain()
{
  //empty
}

void GradientTerrain::HistorySave()
{
  gradient_history.append(field);

  alpha_history.append(ScalarField2(alpha));
  altitude_history.append(ScalarField2(alt));
}

void GradientTerrain::HistoryUndo()
{

  if (gradient_history.size() > 0) {
    field = gradient_history.back();
    gradient_history.pop_back();
    alpha = (alpha_history.back());
    alpha_history.pop_back();
    alt = (altitude_history.back());
    altitude_history.pop_back();
  }
  else {
    qDebug() << "History is empty";
  }
}/*
\brief return the height of the selected terrain point (from the heighfield)
\param i the x coordinate
\param j the y coordinate
*/
double GradientTerrain::HeightAt(const int& i, const int& j)
{
  return heightField.GetHeight(Vector2(i, j));
}


/*!
\brief multiplies the gradient norms by 1.05 for all the points in the selected circles
\param positions the centers of all the selected circles
\param radius the radius of the circles
*/
void GradientTerrain::EnhanceLocalGradient(const QVector<Vector2>& positions, const double& radius)
{
  if (record) {
    ExportPositions(positions, radius, "enhance");
  }
  for (const Vector2 position : positions) {
    Multiply(position, radius, 1.05);
  }
}

/*!
\brief multiplies the gradient norms by 0.95 for all the points of the selected circles
\param positions the centers of all the selected circles
\param radius the radius of the circles
*/
void GradientTerrain::SoftenLocalGradient(const QVector<Vector2>& positions, const double& radius)
{
  if (record) {
    ExportPositions(positions, radius, "soften");
  }
  for (const Vector2 position : positions) {
    Multiply(position, radius, 0.95);
  }
}

/*!
\brief applies a tensor to re-oriente the slopes in a selected zone
\param zone the selected zone
\param orientation the new orientation of the slopes
*/
void GradientTerrain::SlopeOrientationEffect(const QPolygon& zone, const Vector2& orientation, const double& strength)
{
  if (record) {
    ExportZone(zone, "tensor");
  }
  double directionAngle = orientation.Angle() + Math::HalfPi;

  Matrix2 id = Matrix2::Identity;
  Matrix2 directionalTensor = Matrix2::Tensor2(directionAngle);

  const QRect area = zone.boundingRect();
  const QPoint sourceCorner = area.topLeft();

  QPoint sourcePoint;  //the points coordonates in the field object

  Matrix2 tensor = (id + strength * directionalTensor);

  for (int y = 0; y <= area.height(); y++)
  {
    for (int x = 0; x <= area.width(); x++)
    {
      if (zone.containsPoint(sourceCorner + QPoint(x, y), Qt::OddEvenFill)) {
        sourcePoint = VertexInteger(Vector2(sourceCorner.x() + x, sourceCorner.y() + y));
        if (!InsideVertexIndex(sourcePoint))  // the lasso may go outside of the terrain
          continue;
        Vector2 before = field[VertexIndex(sourcePoint.x(), sourcePoint.y())];

        field[VertexIndex(sourcePoint.x(), sourcePoint.y())] = tensor * field[VertexIndex(sourcePoint.x(), sourcePoint.y())];// / (1 + 0.5 * strength);
        field[VertexIndex(sourcePoint.x(), sourcePoint.y())] = field[VertexIndex(sourcePoint.x(), sourcePoint.y())] / Norm(field[VertexIndex(sourcePoint.x(), sourcePoint.y())]) * Norm(before); // norm conservation
      }
    }
  }


}

/*!
\brief copies the gradients from a position to another
\param destinations the lists of all the destinations where we want to copy the points
\param sourceOrigin the center of the source to copy the points
\param destinationOrigin the first point of the destinations
\param radius the radius of the circle around the center of the sources and destinations to see which points we clone or not
*/
void GradientTerrain::CopyGradientLinearZone(const QVector<Vector2>& destinations, const Vector2& sourceOrigin, const Vector2& destinationOrigin, const double& radius)
{
  if (record) {
    ExportStroke(destinations, radius, "copy");
  }
  for (const Vector2 destination : destinations) {
    Clone(destination, sourceOrigin, destinationOrigin, radius);
  }
}

/*!
\brief copies the gradients from a zone to another and blends it in the terrain
\param destinationCenter the center of the destination of the zone where we want to copy the gradients
\param sourceZone the Polygon that outlines the zone we want to copy the gradients from
\param blendTreshold Percentage of the new zone that will be blended with the previous terrain (% of the distance from the center of the zone)
*/
void GradientTerrain::CopyGradientFreeZone(const Vector2& destinationCenter, const QPolygon& sourceZone, const int& blendTreshold)
{
  if (record) {
    ExportZone(sourceZone, "source");
  }
  CloneZone(destinationCenter, sourceZone, blendTreshold);
}

/*!
\brief copies the gradients from a broken line to another
\param sourceLine the list of the points of the sourceLine (the line from which we copy the gradient)
\param destLine the list of the points of the destLine (the line to which we copy the gradient)
\param halfWidth the distance from each side of the broken line where we also copy the gradients
*/
void GradientTerrain::CopyGradientBrokenLine(const QVector<Vector2>& sourceLine, const QVector<Vector2>& destLine, const int& halfWidth)
{
  if (record) {
    ExportStroke(sourceLine, halfWidth, "clonesource");
    ExportStroke(sourceLine, halfWidth, "clonedest");
  }
  CloneBrokenLine(sourceLine, destLine, halfWidth);
}

#include "noise.h"

/*!
\brief Remove all the constraints present in the reconstruction so that all the points are free to move
*/
void GradientTerrain::ForgetConstraints(void) {
  UpdateHeightField();
  for (unsigned int i = 0; i < Nx - 2; i++) {
    for (unsigned int j = 0; j < Ny - 2; j++) {
      double xgrad, ygrad;
      if (i == 0) {
        xgrad = (heightField(i + 1, j) - heightField(i, j));
      }
      else if (i == Nx - 3) {
        xgrad = (heightField(i, j) - heightField(i - 1, j));
      }
      else {
        xgrad = (heightField(i + 1, j) - heightField(i - 1, j)) * 0.5;
      }

      if (j == 0) {
        ygrad = (heightField(i, j + 1) - heightField(i, j));
      }
      else if (j == Ny - 3) {
        ygrad = (heightField(i, j) - heightField(i, j - 1));
      }
      else {
        ygrad = (heightField(i, j + 1) - heightField(i, j - 1)) * 0.5;
      }
      (*this)(i, j) = 1. * Vector2(xgrad, ygrad) / HFMAGN;
      alt(i, j) = .0;
      alpha(i, j) = 1.0;
    }
  }
  UpdateHeightField();
}

/*!
\brief Keep the constraints but synchronize the gradient so that constraints can be removed without any effect
*/
void GradientTerrain::SyncGradient(void) {
  UpdateHeightField();
  for (unsigned int i = 0; i < Nx - 2; i++) {
    for (unsigned int j = 0; j < Ny - 2; j++) {
      double xgrad, ygrad;
      if (i == 0) {
        xgrad = (heightField(i + 1, j) - heightField(i, j));
      }
      else if (i == Nx - 3) {
        xgrad = (heightField(i, j) - heightField(i - 1, j));
      }
      else {
        xgrad = (heightField(i + 1, j) - heightField(i - 1, j)) * 0.5;
      }

      if (j == 0) {
        ygrad = (heightField(i, j + 1) - heightField(i, j));
      }
      else if (j == Ny - 3) {
        ygrad = (heightField(i, j) - heightField(i, j - 1));
      }
      else {
        ygrad = (heightField(i, j + 1) - heightField(i, j - 1)) * 0.5;
      }
      (*this)(i, j) = 1. * Vector2(xgrad, ygrad) / HFMAGN;
      //alt(i, j) = .0;
      //alpha(i, j) = 1.0;
    }
  }
  UpdateHeightField();
}




/*!
\brief Set the altitude of the terrain on a given line
\param line the list of the points of the Line
\param altitude the desired altitude
\param the radius of influence
*/
void GradientTerrain::SetAltitude(const QVector<Vector2>& line, const double& altitude, const double& radius, const double& noise, const double& slope)
{

  if (record) {
    ExportStroke(line, radius, "setaltitude");
  }
  Noise2 n;
  PointCurve2 sourceCurve(line);
  QRect area = AreaInteger();
  ScalarField2 phi(heightField, 0.0);
  //phi.Fill(0.0);
  double freq = 0.02;
  double le = sourceCurve.Size();
  double r = max(1.2, radius);
  // compute the functoin phi that simply interpolates the constraint
  for (int i = 0; i < area.width(); i++)
  {
    for (int j = 0; j < area.height(); j++)
    {
      //double currentAltitude = heightField.at(i, j);
      Vector2 pos = ArrayVertex(i, j);
      //cout << pos << std::endl;
      double t;
      int k;
      double dist = sourceCurve.R(pos, t, k);
      if (dist < r * r) {
        alt(i, j) = altitude + (k + t) * slope / le + (1.0 - 2. * abs(0.5 - n.Value(pos * freq))) * noise * 0.3;

        alpha(i, j) = 0.0;
      }

    }
  }
}


/*!
\brief Set the altitude of the terrain on a given line and also sets the gradient on the sides
\param line the list of the points of the Line
\param altitude the desired altitude
\param the radius of influence
\param sloperight The slope on the right of the line
\param slopeleft The slope on the left of the line
*/
void GradientTerrain::SetAltitudeAndGradient(const QVector<Vector2>& line, const double& altitude, const double& radius, const double& sloperight, const double& slopeleft)
{
  if (line.length() < 2)
    return;

  if (record) {
    ExportStroke(line, radius, "setaltitudegradient");
  }
  Noise2 n;
  PointCurve2 sourceCurve(line);
  QRect area = AreaInteger();
  ScalarField2 phi(heightField, 0.0);
  //phi.Fill(0.0);

  double length = sourceCurve.Size();
  double r = max(1.2, radius);
  // compute the functoin phi that simply interpolates the constraint
  for (int i = 0; i < area.width(); i++)
  {
    for (int j = 0; j < area.height(); j++)
    {
      //double currentAltitude = heightField.at(i, j);
      Vector2 pos = ArrayVertex(i, j);
      //cout << pos << std::endl;
      double t;
      int k;
      int side;
      double dist = sourceCurve.R(pos, t, k, side);
      if (dist < 1.2 * 1.2)
      {
        alt(i, j) = altitude;
        alpha(i, j) = 0.0;
      }
      else if (dist < r * r)
      {
        Vector2 proj = sourceCurve.At(k) + t * (sourceCurve.At(k + 1) - sourceCurve.At(k));
        Vector2 gradientdir;
        gradientdir = pos - proj;

        gradientdir = Normalized(gradientdir);
        if (side == 1)
          gradientdir = gradientdir * slopeleft;
        else
          gradientdir = gradientdir * sloperight;
        //alpha(i, j) = 1.0;

        (*this)(i, j) += Cubic::Smooth(dist, radius * radius) * gradientdir;
      }
    }
  }
}


/*!
\brief Add gradient to the sides of a curve
\param line the list of the points of the Line
\param the radius of influence
\param sloperight The slope on the right of the line
\param slopeleft The slope on the left of the line
*/
void GradientTerrain::AddGradient(const QVector<Vector2>& line, const double& radius, const double& slopeleft, const double& sloperight)
{
  if (line.length() < 2)
    return;

  if (record) {
    ExportStroke(line, radius, "addgradient");
  }
  Noise2 n;
  PointCurve2 sourceCurve(line);
  QRect area = AreaInteger();
  ScalarField2 phi(heightField, 0.0);
  //phi.Fill(0.0);

  double r = max(1.2, radius);
  // compute the functoin phi that simply interpolates the constraint
  for (int i = 0; i < area.width(); i++)
  {
    for (int j = 0; j < area.height(); j++)
    {
      //double currentAltitude = heightField.at(i, j);
      Vector2 pos = ArrayVertex(i, j);
      //cout << pos << std::endl;
      double t;
      int k;
      int side;
      double dist = sourceCurve.R(pos, t, k, side);
      if (dist < r * r)
      {
        Vector2 proj = sourceCurve.At(k) + t * (sourceCurve.At(k + 1) - sourceCurve.At(k));
        Vector2 gradientdir;
        gradientdir = pos - proj;

        gradientdir = Normalized(gradientdir);
        if (side == 1)
          gradientdir = gradientdir * sloperight;
        else
          gradientdir = gradientdir * slopeleft;
        //alpha(i, j) = 1.0;
        double a = Cubic::Smooth(dist, radius * radius);
        (*this)(i, j) += a * gradientdir;
      }
    }
  }
}



void GradientTerrain::ExportPositions(const QVector<Vector2>& line, const double& radius, const QString& suffix) {
  if (line.length() < 2)
    return;
  QRect area = AreaInteger();
  ScalarField2 stroke(heightField.GetArray(), 0.);

  for (const Vector2 center : line) {

    // Compute modification Area
    const QRect area = VertexIntegerArea(Circle2(center, radius));

    for (int y = area.y(); y <= area.y() + area.height(); y++)
    {
      for (int x = area.x(); x <= area.x() + area.width(); x++)
      {
        // Distance between central point and current point
        double u = SquaredNorm(center - ArrayVertex(x, y));

        if (u < radius * radius)
        {
          // We modify the field proportionnaly to the height difference
          stroke[VertexIndex(x, y)] = max(Cubic::Smooth(u, radius * radius), stroke[VertexIndex(x, y)]);
        }
      }
    }
  }
  QString strokename = recordPath + QString::number(recordid).rightJustified(3, '0') + "_" + suffix + ".png";
  stroke.CreateImage().save(strokename);
}


void GradientTerrain::ExportStroke(const QVector<Vector2>& line, const double& radius, const QString& suffix) {
  if (line.length() < 2)
    return;
  PointCurve2 sourceCurve(line);
  QRect area = AreaInteger();
  ScalarField2 stroke(heightField.GetArray(), 0.);

  for (int i = 0; i < area.width(); i++) {
    for (int j = 0; j < area.height(); j++) {

      Vector2 pos = ArrayVertex(i, j);
      //cout << pos << std::endl;
      double t;
      int k;
      double dist = sqrt(sourceCurve.R(pos, t, k));

      if (dist < radius) {

        stroke(i, j) = 1.0;
      }
    }
  }
  QString strokename = recordPath + QString::number(recordid).rightJustified(3, '0') + "_" + suffix + ".png";
  stroke.CreateImage().save(strokename);
}

void GradientTerrain::ExportZone(const QPolygon& poly, const QString& suffix)
{
  const QRect area = poly.boundingRect();
  const QPoint sourceCorner = area.topLeft();
  ScalarField2 stroke(heightField.GetArray(), 0.);

  for (int y = 0; y <= area.height(); y++)   //relative position of the point in the bounding rect of the zone to clone
  {
    for (int x = 0; x <= area.width(); x++)
    {
      QPoint pos = VertexInteger(Vector2(sourceCorner.x() + (double)x, sourceCorner.y() + (double)y));
      if (poly.containsPoint(sourceCorner + QPoint(x, y), Qt::OddEvenFill) && stroke.InsideVertexIndex(pos))
      {
        stroke(pos) = 1.0;
      }
    }
  }

  QString strokename = recordPath + QString::number(recordid).rightJustified(3, '0') + "_" + suffix + ".png";
  stroke.CreateImage().save(strokename);
}

void GradientTerrain::ExportMap(const ScalarField2& map, const QString& suffix)
{
  QString mapname = recordPath + QString::number(recordid).rightJustified(3, '0') + "_" + suffix + ".png";
  map.CreateImage().save(mapname);
}


/*!
\brief Apply a tensor in the direction of the stroke with a given radius and a mountain shape
\param line the list of the points of the contour
\param the radius of influence
*/
void GradientTerrain::Mountain(const QVector<Vector2>& line, const double& radius, const double& slope)
{
  if (line.length() < 2)
    return;
  if (record) {
    ExportStroke(line, radius, "mountain");
  }
  QuadricCurve2Set sourceCurve(line);
  Noise2 noise;
  QRect area = AreaInteger();
  double pente = slope;
  HeightField hf(Box2(40.0, 40.0), Nx, Ny, 0.0);

  for (int i = 0; i < hf.GetSizeX(); i++) {
    for (int j = 0; j < hf.GetSizeY(); j++) {
      // EGUERIN CHECK USE : EN PLUS ICI LES COORDONN2ES X ET Y VONT DE -20 A 20 et PAS DE -40 A 40 : EST CE VOULU ?
      Vector2 p = hf.ArrayVertex(i, j) * 0.5;

      hf[hf.VertexIndex(i, j)] = 0.2 * (2.1 * noise.Value(0.5 * p) + noise.Value(1. * p) + 0.53 * noise.Value(2. * p) + 0.27 * noise.Value(4. * p) + 0.13 * noise.Value(8. * p) + 0.7 * noise.Value(16. * p)) / HFMAGN;

    }
  }
  hf *= 1.2;

  VectorField2 g = hf.Gradient();

  for (int i = 0; i < area.width(); i++) {
    for (int j = 0; j < area.height(); j++) {

      Vector2 pos = ArrayVertex(i, j);
      //cout << pos << std::endl;
      double t;
      int k;
      double dist = sqrt(sourceCurve.R(pos, t, k));

      if (dist < radius) {
        double coef = dist / radius;
        if (coef > 1.0)
          coef = 1.0;
        coef = 1.0 - coef;
        double coeft = 1.4;
        Vector2 proj = sourceCurve(k)(t);
        Vector2 direction = Normalized(pos - proj);
        //cout << direction << std::endl;
        Matrix2 ani = Matrix2::Tensor2(direction.Angle() + Math::HalfPi);

        Matrix2 tenseur = Matrix2::Identity + coeft * ani;
        (*this)(i, j) = (*this)(i, j) - slope * pow(coef, 0.5) * tenseur * g(i, j) / (0.8 + 0.4 * coeft) + Cubic::Smooth(coef) * pente / HFMAGN * direction;
      }
    }
  }
}

/*!
\brief Apply a tensor in the direction of the stroke with a given radius and a mountain shape
\param line the list of the points of the contour
\param the radius of influence
*/
void GradientTerrain::TensorNoise(const QVector<Vector2>& line, const double& radius, const double& amplitude)
{
  if (line.length() < 2)
    return;
  if (record) {
    ExportStroke(line, radius, "tensornoise");
  }
  QuadricCurve2Set sourceCurve(line);
  Noise2 noise;
  QRect area = AreaInteger();
  HeightField hf(Box2(40.0, 40.0), Nx, Ny, 0.0);

  for (int i = 0; i < hf.GetSizeX(); i++)
  {
    for (int j = 0; j < hf.GetSizeY(); j++)
    {
      Vector2 p = ArrayVertex(i, j);

      hf[hf.VertexIndex(i, j)] = 0.2 * (2.1 * noise.Value(0.5 * p) + noise.Value(1. * p) + 0.53 * noise.Value(2. * p) + 0.27 * noise.Value(4. * p) + 0.13 * noise.Value(8. * p) + 0.7 * noise.Value(16. * p)) / HFMAGN;

    }
  }
  hf *= 1.2;

  VectorField2 g = hf.Gradient();

  for (int i = 0; i < area.width(); i++) {
    for (int j = 0; j < area.height(); j++) {

      Vector2 pos = ArrayVertex(i, j);
      //cout << pos << std::endl;
      double t;
      int k;
      double dist = sqrt(sourceCurve.R(pos, t, k));

      if (dist < radius) {
        double coef = dist / radius;
        if (coef > 1.0)
          coef = 1.0;
        coef = 1.0 - coef;
        double coeft = 1.4;
        Vector2 direction = sourceCurve(k).Tangent(t);
        Matrix2 ani = Matrix2::Tensor2(direction.Angle() + Math::HalfPi);

        Matrix2 tenseur = Matrix2::Identity + coeft * ani;
        (*this)(i, j) = (*this)(i, j) + 0.05 * amplitude * pow(coef, 0.5) * tenseur * g(i, j);
      }
    }
  }
}

/*!
\brief Add a noise with a given radius
\param line the list of the points of the contour
\param the radius of influence
\param the amplitude
*/
void GradientTerrain::AddSomeNoize(const QVector<Vector2>& line, const double& radius, const double& amplitude)
{
  if (line.length() < 2)
    return;
  if (record) {
    ExportStroke(line, radius, "tensornoise");
  }
  QuadricCurve2Set sourceCurve(line);
  Noise2 noise;
  QRect area = AreaInteger();
  HeightField hf(Box2(40.0, 40.0), Nx, Ny, 0.0);

  for (int i = 0; i < hf.GetSizeX(); i++)
  {
    for (int j = 0; j < hf.GetSizeY(); j++)
    {
      Vector2 p = ArrayVertex(i, j);


      hf[hf.VertexIndex(i, j)] = 0.2 * (2.1 * noise.Value(0.5 * p) + noise.Value(1. * p) + 0.53 * noise.Value(2. * p) + 0.27 * noise.Value(4. * p) + 0.13 * noise.Value(8. * p) + 0.7 * noise.Value(16. * p)) / HFMAGN;

    }
  }
  hf *= 1.2;

  VectorField2 g = hf.Gradient();

  for (int i = 0; i < area.width(); i++) {
    for (int j = 0; j < area.height(); j++) {

      Vector2 pos = ArrayVertex(i, j);
      //cout << pos << std::endl;
      double t;
      int k;
      double dist = sqrt(sourceCurve.R(pos, t, k));

      if (dist < radius) {
        double coef = dist / radius;
        if (coef > 1.0)
          coef = 1.0;
        coef = 1.0 - coef;

        (*this)(i, j) = (*this)(i, j) + 0.05 * amplitude * pow(coef, 0.5) * g(i, j);
      }
    }
  }
}




/*!
\brief Apply a tensor in the direction of the stroke with a given radius and a mountain shape
\param line the list of the points of the contour
\param the radius of influence
*/
void GradientTerrain::TensorBrush(const QVector<Vector2>& line, const double& radius, const double& strength)
{
  if (line.length() < 2)
    return;
  if (record) {
    ExportStroke(line, radius, "tensorbrush");
  }
  QuadricCurve2Set sourceCurve(line);
  Noise2 noise;
  QRect area = AreaInteger();

  HeightField hf(Box2(40.0, 40.0), Nx, Ny, 0.0);


  for (int i = 0; i < area.width(); i++)
  {
    for (int j = 0; j < area.height(); j++)
    {

      Vector2 pos = ArrayVertex(i, j);
      //cout << pos << std::endl;
      double t;
      int k;
      double dist = sqrt(sourceCurve.R(pos, t, k));

      if (dist < radius)
      {
        double coef = dist / radius;
        if (coef > 1.0)
          coef = 1.0;
        coef = 1.0 - coef;
        Vector2 direction = sourceCurve(k).Tangent(t);        //Normalized(pos - proj);
        //cout << direction << std::endl;
        Matrix2 ani = Matrix2::Tensor2(direction.Angle() + Math::HalfPi);

        Matrix2 tenseur = Matrix2::Identity + strength * ani;
        (*this)(i, j) = (1.0 - pow(coef, 0.5)) * (*this)(i, j) + pow(coef, 0.5) * tenseur * (*this)(i, j);
      }
    }
  }
}



#include "diffusionterrain.h"

/*!
\brief Look for rivers and put absolute constraints on them

*/
void GradientTerrain::FreezeRivers(const double& riverWidth)
{
  HeightField hf = heightField;
  hf.CompleteBreach();
  hf.FillDepressions(0.0);
  QRect area = AreaInteger();

  ScalarField2 hydroSquelette = hf.StreamArea().Sqrt();
  hydroSquelette.SetRange(0., 1.);
  QRect r = hydroSquelette.GetBox().GetQtRect().toRect();

  double a, b;
  hydroSquelette.GetRange(a, b);
  double limit = 0.1;
  ScalarField2 hydroMap = hydroSquelette;
  for (int i = 0; i <= area.width(); i++) {
    for (int j = 0; j <= area.height(); j++) {

      //double limit = 0.1;	//the closer to 0 the more things will be considered as river, the closer to 1 the less river there will be
      if (hydroSquelette(i, j) > limit) {	//where there is a river we put a 0 on the line (arbitrairy limit value to consider it's a river)
        hydroMap(i, j) = 0;
      }
      else {
        hydroMap(i, j) = 1;
      }
    }
  }
  ScalarField2 distToRivers = hydroMap.DistanceTransform(0.5);
  distToRivers.Sqrted();
  if (record) {
    ExportMap(distToRivers, "riverdist");
  }
  for (int i = 0; i < nx; i++) {
    for (int j = 0; j < ny; j++) {
      //QPoint p = VertexInteger(Vector2(i, j));

      double riverdist = distToRivers(i, j);
      if (riverdist < riverWidth) {
        alt(i, j) = heightField(i, j) / HFMAGN;
        alpha(i, j) = 0.0;
      }
    }
  }
}


/*!
\brief Look for rivers and put absolute constraints on them

*/
void GradientTerrain::FreezeRivers(const QPolygon& zone, const double& riverWidth)
{
  HeightField hf = heightField;
  hf.CompleteBreach();
  hf.FillDepressions(0.0);
  if (record) {
    ExportZone(zone, "freezeriverszone");
  }
  const QRect area = zone.boundingRect();
  const QRect areatotal = AreaInteger();
  const QPoint sourceCorner = area.topLeft();

  ScalarField2 hydroSquelette = hf.StreamArea().Sqrt();
  hydroSquelette.SetRange(0., 1.);
  QRect r = hydroSquelette.GetBox().GetQtRect().toRect();

  double limit = 0.1;
  ScalarField2 hydroMap = hydroSquelette;
  for (int i = 0; i <= areatotal.width(); i++)
  {
    for (int j = 0; j <= areatotal.height(); j++)
    {

      //double limit = 0.1;	//the closer to 0 the more things will be considered as river, the closer to 1 the less river there will be
      if (hydroSquelette(i, j) > limit) {	//where there is a river we put a 0 on the line (arbitrairy limit value to consider it's a river)
        hydroMap(i, j) = 0;
      }
      else
      {
        hydroMap(i, j) = 1;
      }
    }
  }
  ScalarField2 distToRivers = hydroMap.DistanceTransform(0.5);
  distToRivers.Sqrted();
  if (record) {
    ExportMap(distToRivers, "riverdist");
  }
  for (int i = 0; i < area.width(); i++)
  {
    for (int j = 0; j < area.height(); j++)
    {
      //QPoint p = VertexInteger(Vector2(i, j));

      if (zone.containsPoint(sourceCorner + QPoint(i, j), Qt::OddEvenFill))
      {
        QPoint p = VertexInteger(Vector2(sourceCorner.x() + i, sourceCorner.y() + j));
        if (!InsideVertexIndex(p))  // the lasso may go outside of the terrain
          continue;
        double riverdist = distToRivers(p);
        if (riverdist < riverWidth)
        {
          alt(p) = heightField(p) / HFMAGN;
          alpha(p) = 0.0;
        }
      }
    }
  }
}




#include "ppa.h"

/*!
\brief Look for ridges using PPA and put absolute constraints on them

*/
void GradientTerrain::FreezeRidges()
{
  HeightField hf = heightField;

  QRect area = AreaInteger();

  PPA* ppa = new PPA(hf);
  ppa->compute(20);
  const std::vector<PPA::RidgeSegment>& ridges = ppa->getSegmentsInPrunedMST();
  for (const PPA::RidgeSegment& s : ridges)
  {
    QPoint p1 = s.first;
    QPoint p2 = s.second;
    alt(p1) = heightField(p1) / HFMAGN;
    alpha(p1) = 0.0;
    alt(p2) = heightField(p2) / HFMAGN;
    alpha(p2) = 0.0;
  }
}

/*!
\brief Freeze portions of the terrain
\param positions the centers of all the selected circles
\param radius the radius of the circles
*/
void GradientTerrain::FreezeTool(const QVector<Vector2>& positions, const double& radius) {

  if (record) {
    ExportPositions(positions, radius, "freeze");
  }
  for (const Vector2 center : positions) {
    const QRect area = VertexIntegerArea(Circle2(center, radius));

    for (int y = area.y(); y <= area.y() + area.height(); y++)
    {
      for (int x = area.x(); x <= area.x() + area.width(); x++)
      {
        // Distance between central point and current point
        double u = SquaredNorm(center - ArrayVertex(x, y));

        if (u < radius * radius)
        {
          // We modify the field proportionnaly to the height difference
          alpha[VertexIndex(x, y)] = 0.0;
          alt[VertexIndex(x, y)] = heightField[VertexIndex(x, y)] / HFMAGN;
        }

      }
    }
  }
}/*!
\brief Move the constraint of a give altitude shift
\param positions the centers of all the selected circles
\param radius the radius of the circles
*/
void GradientTerrain::MoveConstraint(const QVector<Vector2>& positions, const double& radius, const double& shift) {

  if (record) {
    ExportPositions(positions, radius, "moveconstraint");
  }
  ScalarField2 a(alt);
  a.Fill(0.);
  for (const Vector2 center : positions) {
    const QRect area = VertexIntegerArea(Circle2(center, radius));

    for (int y = area.y(); y <= area.y() + area.height(); y++)
    {
      for (int x = area.x(); x <= area.x() + area.width(); x++)
      {
        // Distance between central point and current point
        double u = SquaredNorm(center - ArrayVertex(x, y));

        if (u < radius * radius && alpha[VertexIndex(x, y)] == 0.0)
        {
          a[VertexIndex(x, y)] = max(a[VertexIndex(x, y)], Cubic::Smooth(u, radius * radius));
        }
      }
    }
  }

  for (int y = 0; y < a.GetSizeY(); y++)
  {
    for (int x = 0; x < a.GetSizeX(); x++)
    {
      // We alter the altitude
      alt(x, y) += a(x, y) * shift;
    }
  }

}

/*!
\brief UnFreeze portions of the terrain
\param positions the centers of all the selected circles
\param radius the radius of the circles
*/
void GradientTerrain::RemoveConstraint(const QVector<Vector2>& positions, const double& radius) {

  if (record) {
    ExportPositions(positions, radius, "unfreeze");
  }
  for (const Vector2 center : positions) {
    const QRect area = VertexIntegerArea(Circle2(center, radius));

    for (int y = area.y(); y <= area.y() + area.height(); y++)
    {
      for (int x = area.x(); x <= area.x() + area.width(); x++)
      {
        // Distance between central point and current point
        double u = SquaredNorm(center - ArrayVertex(x, y));

        if (u < radius * radius)
        {
          // We cancel the constraint
          alpha[VertexIndex(x, y)] = 1.0;
        }

      }
    }
  }
}


/*!
\brief Look for ridges using PPA and put absolute constraints on them

*/
void GradientTerrain::FreezeRidges(const QPolygon& zone)
{
  HeightField hf = heightField;

  const QRect area = zone.boundingRect();
  const QPoint corner = area.topLeft();


  PPA* ppa = new PPA(hf);
  ppa->compute(20);
  const std::vector<PPA::RidgeSegment>& ridges = ppa->getSegmentsInPrunedMST();
  for (const PPA::RidgeSegment& s : ridges)
  {
    QPoint p1 = s.first;
    QPoint p2 = s.second;
    Vector2 p1f = ArrayVertex(p1);
    QPoint p1i(p1f[0], p1f[1]);
    //QPoint p1i = VertexInteger(Vector2(p1.x(),p1.y()));
    if (zone.containsPoint(p1i, Qt::OddEvenFill))
    {
      alt(p1) = heightField(p1) / HFMAGN;
      alpha(p1) = 0.0;
    }
    /*if (zone.containsPoint(p2, Qt::OddEvenFill))
    {
        alt(p2) = heightField(p2) / HFMAGN;
        alpha(p2) = 0.0;
    }*/
  }
}


#include "gputerrainsimu.h"

/*!
\brief Uses GPUErosion on the HF to produce hydrolic erosion
*/
void GradientTerrain::GPUErosion()
{
  HeightField hf = heightField;
  GPUHydraulicErosionParticle erosion;

  std::cout << "Box hf " << hf.GetBox() << std::endl;
  erosion.Init(hf);
  erosion.Step(5);
  hf.Fill(.0);
  erosion.GetData(hf);

  hf *= hf.CellDiagonal()[0] / HFMAGN;
  VectorField2 gradphi = hf.Gradient();
  // TEST EGUERIN TRY USE Sub(gradphi); (s'applique à this si j'ai bien suivi ton code)

  for (int i = 0; i < nx; i++) {
    for (int j = 0; j < ny; j++) {
      (*this)(i, j) = gradphi(i, j);
    }
  }
}


/*!
\brief computes the Heightfield from the gradients' VectorField2
*/
void GradientTerrain::UpdateHeightField()
{
  for (unsigned int i = 0; i < Nx - 2; i++)
  {
    for (unsigned int j = 0; j < Ny - 2; j++)
    {
      double gxx = .0;
      if (i < Nx - 3)
        gxx = at(i + 1, j)[0] - at(i, j)[0];
      double gyy = .0;
      if (j < Ny - 3)
        gyy = at(i, j + 1)[1] - at(i, j)[1];
      lap(i, j) = 0.25 * (gxx + gyy);
    }
  }

    SimpleGeometricMultigridFloat poisson(alpha, alt, lap);
  poisson.InitGL();
  poisson.Solve();

  heightField = poisson.GetResult();
  if (record) {
    //qDebug("Export terrain");

    QString terrainname = recordPath + QString::number(recordid).rightJustified(3, '0') + "_terrain" + ".png";
    QString altname = recordPath + QString::number(recordid).rightJustified(3, '0') + "_alt" + ".png";
    //heightField.CreateImage(0., HFMAGN, true).save(terrainname);
    {
      double a, b;
      heightField.GetRange(a, b);
      double m = max(abs(a), abs(b)); // = 0.5; // default range
      heightField.CreateImage(-m, m).save(terrainname);
      alt.CreateImage(-m, m).save(altname);
    }

    QString maskname = recordPath + QString::number(recordid).rightJustified(3, '0') + "_mask" + ".png";
    alpha.CreateImage().save(maskname);

    QString lapname = recordPath + QString::number(recordid).rightJustified(3, '0') + "_lap" + ".png";
    {
      double a, b;
      lap.GetRange(a, b);
      double m = max(abs(a), abs(b));
      lap.CreateImage(-m, m).save(lapname);
    }

    recordid++;
  }
  heightField *= HFMAGN;
}

#include <QtCore/QDateTime>
#include <QtCore/QDir>

void GradientTerrain::ToggleRecord(int val)
{
  if (!record && val) {
    qDebug() << "Toggle record -> on";
    QDateTime date = QDateTime::currentDateTime();
    recordPath = "./export/" + date.toString("yyyy.MM.dd-hhmm") + "/";
    qDebug() << recordPath;
    if (!QDir(recordPath).exists())
    {
      QDir().mkpath(recordPath);
      qDebug() << "mkpath ";
    }
  }
  record = val;
}

