// Vector sets

#include "vectorset.h"
#include <cassert>

/*!
\class VectorSet2 vectorset.h
\brief A simple set of vectors in the plane.

The class provides range-based for loops.

\sa VectorSet

\ingroup PlanarGroup
*/

/*!
\brief Create a set of vectors.
\param s The set.
*/
VectorSet2::VectorSet2(const QVector<Vector2>& s) :v(s)
{
}
