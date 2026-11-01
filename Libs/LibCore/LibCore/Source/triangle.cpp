// Triangle

#include "triangle.h"

// Used for drawing
#include <QtGui/QPolygonF>
/*!
\brief Compute the normal vector of the triangle, and scale the normal using its area.

This function is less expensive than Triangle::Normal() as it does not require normalizing the cross product of the edge vectors.

It is useful for computing the vertex normals of a triangle mesh by averaging the face normals of the triangles and weighting
the influence of the different triangles with their area.
\sa Triangle::Normal()
*/
Vector Triangle::AreaNormal() const
{
  return 0.5 * ((p[1] - p[0]) / (p[2] - p[0]));
}