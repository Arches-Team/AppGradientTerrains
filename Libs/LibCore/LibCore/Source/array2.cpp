// Arrays


#include "array.h"

/*!
\class Array2 array.h
\brief A base two-dimensional array structure.

\image html array.png

An array is a two-dimensional rectangle virtually decomposed into cells.

\ingroup StructureGroup
*/

const QPoint Array2::next[8] = { QPoint(1, 0), QPoint(1, 1), QPoint(0, 1), QPoint(-1, 1), QPoint(-1, 0), QPoint(-1, -1), QPoint(0, -1), QPoint(1, -1) };
const double Array2::length[8] = { 1.0, sqrt(2.0), 1.0, sqrt(2.0), 1.0, sqrt(2.0), 1.0, sqrt(2.0) };
const double Array2::inverselength[8] = { 1.0, 1.0 / sqrt(2.0), 1.0, 1.0 / sqrt(2.0), 1.0, 1.0 / sqrt(2.0), 1.0, 1.0 / sqrt(2.0) };

/*!
\brief Empty array, with empty box.
*/
Array2::Array2() :Box2(Box2::Null), nx(0), ny(0), celldiagonal(Vector2::Null), inversecelldiagonal(Vector2::Null)
{
}

/*!
\brief Create the lattice structure.
\param box The box.
\param x,y Size of the lattice.
*/
Array2::Array2(const Box2& box, int x, int y) :Box2(box), nx(x), ny(y), celldiagonal((b[0] - a[0]) / (nx - 1), (b[1] - a[1]) / (ny - 1))
{
  inversecelldiagonal = celldiagonal.Inverse();
}/*!
\brief Return the cell diagonal vector.
*/
Vector2 Array2::CellDiagonal() const
{
  return celldiagonal;
}/*!
\brief Compute the integer coordinates of the vertices embedding a box.

The coordinates are clamped to the size of the array.
\param box %Box.
*/
QRect Array2::VertexIntegerArea(const Box2& box) const
{
  QPoint pa = VertexInteger(box[0]);
  QPoint pb = VertexInteger(box[1]);

  // Rectangle
  QRect area(pa.x(), pa.y(), pb.x() - pa.x() + 1, pb.y() - pa.y() + 1);

  // Limit to domain
  QRect mask(0, 0, nx - 1, ny - 1);
  return area.intersected(mask);
}

/*!
\brief Compute the integer coordinates of the vertices embedding a circle.

The coordinates are clamped to the size of the array.
\param circle %Circle.
*/
QRect Array2::VertexIntegerArea(const Circle2& circle) const
{
  return VertexIntegerArea(circle.GetBox());
}