// MayaGeometryAll

#include "maya.h"

/*!
\class MayaGeometryAll maya.h
\brief Set of instances along with their frame lists.

\ingroup MayaCore
*/

/*!
\brief Creates a set of geometries from a unique geometry.

This function creates a single geometry entry and a unique reference with identity frame.
\param mg Input geometry.
*/
MayaGeometryAll::MayaGeometryAll(const MayaGeometry& mg)
{
  Append(mg);
}/*!
\brief Add an instance to the scene.

This function checks if the object already exists in the collection of models.
If it already exists, it simply creates a reference to it, otherwise it creates a new entry.
\param in The instance.
*/
void MayaGeometryAll::Append(const MayaGeometry& in)
{
  if (instances.find(in.GetName()) != instances.end())
  {
    instances[in.GetName()].Append(FrameScaled::Id);
  }
  else
  {
    instances[in.GetName()] = MayaGeometrySet(in, FrameScaled::Id);
  }
}
