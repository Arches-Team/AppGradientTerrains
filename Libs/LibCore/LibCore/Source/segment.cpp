// Segment

#include "segment.h"


/*!
\class Segment segment.h
\brief A segment defined by its end vertices.

%Axis is a similar independent class, which also stores the
normalized axis vector and its length.
%Lines are implemented by the class Line.

<P><I>How do I compute the intersection of two segments?</I>
<BR>In general, segments do not intersect, use the distance function.
\code
Segment a(Vector(0,1,-1),Vector(-1,0,3));
Segment b(Vector(0,1,-1),Vector(-1,0,3));
double r=a.R(b);
\endcode
Alternatively, it is possible to compute the intersection between two
segments in the plane, using Segment2::Intersect(const Segment2&).

\ingroup KernelGroup
*/

/*!
\brief Create a segment from a segment in the plane.
\param s Plane segment.
\param u,v Elevations.
*/
Segment::Segment(const Segment2& s, const double& u, const double& v) :Segment(s.Vertex(0).ToVector(u), s.Vertex(1).ToVector(v))
{
}