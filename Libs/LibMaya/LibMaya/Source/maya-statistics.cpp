// MayaMaterial
#include "mayageometry.h"

/*!
\class MayaStatistics maya.h
\brief Statistics about objects and instances.

\ingroup MayaCore
*/

/*!
\brief Create statistics.
\param objects Number of unique objects.
\param tobjects, vobjects Number of triangles and vertices of unique objects.
\param instances Number of instances.
\param tinstances, vinstances Number of triangles and vertices for instances.
*/
MayaStatistics::MayaStatistics(int objects, int tobjects, int vobjects, int instances, int tinstances, int vinstances)
{
  s[0] = objects;
  s[1] = tobjects;
  s[2] = vobjects;
  s[3] = instances;
  s[4] = tinstances;
  s[5] = vinstances;
}/*
\brief Clear the statistics of all the instances.
*/
void MayaStatistics::ClearInstances()
{
  s[3] = s[4] = s[5] = 0;
}

/*
\brief Add one or more instances to the statistics.
\param n Number of instances.
*/
void MayaStatistics::AddInstances(int n)
{
  s[3] += n;
  s[4] += n*s[1];
  s[5] += n*s[2];
}