// Maya

#include "mayageometry.h"
/*!
\brief Create a set of geometric instances.

One reference to the geometric instance is created using the argument frame.

\param mg Geometric object.
\param mf Frame.
*/
MayaGeometrySet::MayaGeometrySet(const MayaGeometry& mg, const FrameScaled& mf) :MayaGeometry(mg)
{
  MayaGeometrySet::frames.clear();
  MayaGeometrySet::frames.append(mf);
}/*!
\brief Adds a frame to the list of instances.
\param frame The frame.
*/
void MayaGeometrySet::Append(const FrameScaled& frame)
{
  frames.append(frame);
}/*!
\brief Merge all the instances into a signel Maya geometry structure.
*/
MayaGeometry MayaGeometrySet::Collapse() const
{
  // Complete set
  MayaGeometry set;
  set.SetName(name + "_c");
  set.SetMaterial(mat);

  // Escape if there are no instances
  if (frames.size() == 0) return set;

  // Collapse all instances into the same geometry
  for (int i = 0; i < frames.size(); i++)
  {
    MayaGeometry truc(name, vertex, normal, UVmap, indexes);
    set.Merge(truc.Transform(frames.at(i)));
  }

  return set;
}/*!
\brief Compute the statistics of the geometry set.
\return The statistics.
*/
MayaStatistics MayaGeometrySet::GetStatistics() const
{
  return MayaStatistics(1, vertex.size(), indexes.size() / 3, frames.size(), frames.size() * vertex.size(), frames.size() * indexes.size() / 3);
}