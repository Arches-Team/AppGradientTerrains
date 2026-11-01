// Frames 

#include "frame.h"

/*!
\class Frame frame.h
\brief Solid transformations.

The class groups the rotation matrix and the translation vector.

The orientation of the frame should be defined by the appropriate rotation matrix.

The inverse transformation is implicitly characterized as the transposed matrix and
the opposite translation vector.

Conversion constructors allow the following:
\code
  Frame f = Frame2::Rotation(Math::Pi/4.0); // Rotation around z-axis
\endcode
\ingroup KernelGroup
*/

const Frame Frame::Id(Matrix::Identity, Vector::Null);

/*!
\brief Creates a frame given a rotation matrix and a translation vector.
\param r Frame matrix.
\param t Translation vector.
*/
Frame::Frame(const Matrix& r, const Vector& t) :r(r), t(t)
{
}