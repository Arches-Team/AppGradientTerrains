// Maya

#include "maya.h"
#include "plane.h"

/*!
\class MayaPlane maya.h
\brief A simple plane with lines for locating objects in the MayaWidget.

\ingroup MayaCore
*/

const double MayaPlane::epsilon = 0.00001;

/*!
\brief Create the plane.
\param box Rectangle.
\param z Elevation.
\param l Distance between lines.
*/
MayaPlane::MayaPlane(const Box2& box, const double& z, const double& l) :area(box), height(z), line(l)
{}

/*!
\brief Destructor.
*/
MayaPlane::~MayaPlane()
{
  delete planeRendererQuad;
  delete planeRendererWhiteLine;
}

/*!
\brief Release the GPU resources of the renderers, which will be created again by the next rendering.

The OpenGL context of the widget must be current.
*/
void MayaPlane::DeleteRenderers()
{
  if (planeRendererQuad != nullptr)
  {
    planeRendererQuad->DeleteBuffers();
    delete planeRendererQuad;
    planeRendererQuad = nullptr;
  }
  if (planeRendererWhiteLine != nullptr)
  {
    planeRendererWhiteLine->DeleteBuffers();
    delete planeRendererWhiteLine;
    planeRendererWhiteLine = nullptr;
  }
}

/*!
\brief Initialize the renderers
*/
void MayaPlane::InitRenderer()
{
  //Create the transparent quad of the plane
  Vector points[4] = { Vector(area[0][0], area[0][1], Z()), Vector(area[0][0], area[1][1], Z()),Vector(area[1][0], area[0][1], Z()),Vector(area[1][0], area[1][1], Z()) };

  planeRendererQuad = new MayaSimpleRenderer(points, 4, Color(0.87, 0.92, 0.97, 0.2), GL_TRIANGLE_STRIP);

  Vector2 d = area.Diagonal() + Vector(0.01);

  if (line == 0.0) return;

  const int nx = int(d[0] / line);
  const int ny = int(d[1] / line);

  //Create the white and black line of the plane
  Segment* q = new Segment[nx + ny + 2];

  double z = height + epsilon;
  int n = 0;
  for (int i = 0; i < nx; i++)
  {
    q[n++] = Segment(area.GetSegment(Math::Unit(i, nx), false), z, z);
    /*
   double x = Math::Lerp(area[0][0], area[1][0], );
    q[n++] = Vector(x, area[0][1], z);
    q[n++] = Vector(x, area[1][1], z);
    */
  }

  for (int i = 0; i < ny; i++)
  {
    q[n++] = Segment(area.GetSegment(Math::Unit(i, ny), true), z, z);
    /*
    double y = Math::Lerp(area[0][1], area[1][1], Math::Unit(i, ny));
    q[n++] = Vector(area[0][0], y, Z() + epsilon);
    q[n++] = Vector(area[1][0], y, Z() + epsilon);
    */
  }

  planeRendererWhiteLine = new MayaSimpleRenderer(q, n, Color(0.62, 0.75, 0.92), GL_LINES);

  delete[] q;
}

/*!
\brief Return the height.
*/
double MayaPlane::Z() const
{
  return height;
}/*!
\brief Intersect a ray with the plane.
\param ray The ray.
\param t Returned intersection depth.
*/
bool MayaPlane::Intersect(const Ray& ray, double& t) const
{
  // Build an infinite horizontal plane with upward normal Z at elevation 'height',
  // then delegate to Plane::Intersect which solves dot(Z, ray(t) - p) = 0 for t
  // and returns true only when the intersection lies in front of the ray origin.
  return Plane(Vector::Z, Vector(0.0, 0.0, height)).Intersect(ray, t);
}/*!
\brief Render the horizontal plane.
*/
void MayaPlane::Render(Camera c, int width, int height)
{
  if (planeRendererQuad == nullptr)
    InitRenderer();

  planeRendererQuad->SetCamera(c, width, height);
  planeRendererQuad->Draw();
  if (planeRendererWhiteLine != nullptr)
  {
    planeRendererWhiteLine->SetCamera(c, width, height);
    planeRendererWhiteLine->Draw();
  }
}