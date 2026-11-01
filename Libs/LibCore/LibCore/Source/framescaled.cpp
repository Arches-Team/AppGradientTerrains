// Frames 

#include "frame.h"

/*!
\class FrameScaled frame.h
\brief A transformation defined as the composition of a rotation, a translation and a scale.

The data-structure groups the rotation matrix, the translation and scaling vectors.

The orientation of the frame should be defined by the appropriate rotation matrix.

In general, a FrameScaled is defined by its rotation matrix, and a translation and scaling vector:
\code
FrameScaled frame(Matrix::Rotation(Vector( Math::Pi/4.0,0.0, 0.0)),Vector(0.0),Vector(1.0,1.0,2.0));
\endcode

When there is no rotation, the rotation matrix can be defined using the identity matrix:
\code
FrameScaled frame(Matrix::Identity,Vector(-2.0,4.0,0.0),Vector(2.0,1.0,1.0));
\endcode
It is also possible and simpler to use:
\code
FrameScaled frame(Vector(-2.0,4.0,0.0),Vector(2.0,1.0,1.0));
\endcode

\ingroup ExtendedKernelGroup
*/

const FrameScaled FrameScaled::Id(Matrix(Vector(1.0)));

/*!
\brief Creates a frame.
\param r Rotation matrix.
\param t Translation vector.
\param s Scaling vector.
*/
FrameScaled::FrameScaled(const Matrix& r, const Vector& t, const Vector& s) :Frame(r, t), s(s)
{
}

/*!
\brief Creates a frame given the translation and scaling vectors.
\param t Translation.
\param s Scaling vector.
*/
FrameScaled::FrameScaled(const Vector& t, const Vector& s) :FrameScaled(Matrix::Identity, t, s)
{
}/*!
\brief Get scaling vector from transformation.
*/
Vector FrameScaled::S() const
{
  return s;
}

/*!
\brief Transforms a given input point.
\param p Point.
*/
Vector FrameScaled::Transform(const Vector& p) const
{
  return Frame::Transform(p.Scaled(s));
}

/*!
\brief Transforms a given input normal.
\param n Normal vector.
*/
Vector FrameScaled::TransformDirection(const Vector& n) const
{
  // Apply rotation
  return Frame::TransformDirection(n.Scaled(s.Inverse()));
}/*!
\brief Creates a translation transformation.

\param t Translation vector.
*/
FrameScaled FrameScaled::Translation(const Vector& t)
{
  return FrameScaled(Matrix::Identity, t);
}