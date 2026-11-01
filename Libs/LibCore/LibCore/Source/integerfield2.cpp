// Fields

#include "arrayinteger.h"

/*!
\class Array2I arrayinteger.h
\brief A base two-dimensional array of integers.

\ingroup StructureGroup
*/

/*!
\brief Create the field structure.
\param a The array.
\param v Default value of field.
*/
Array2I::Array2I(const Array2& a, int v) :Array2(a)
{
  field.fill(v, nx * ny);
}

/*!
\brief Create the field structure.
\param box The box.
\param x,y Size of the array.
\param v Default value of field.
*/
Array2I::Array2I(const Box2& box, int x, int y, int v) :Array2I(Array2(box, x, y), v)
{}/*!
\brief Empty.
*/
Array2I::~Array2I()
{}/*!
\brief Return the field value at a given array vertex.
\param i,j Integer coordinates of the vertex.
\sa at(int,int)
*/
int Array2I::Value(int i, int j) const
{
  return field.at(VertexIndex(i, j));
}