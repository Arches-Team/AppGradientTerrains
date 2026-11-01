// Maya

#include "maya.h"
/*!
\brief Render the set of instances.
\param gpu Set of parameters.
*/
void MayaGpuAll::Render(GpuParameter& gpu)
{
  // Activation du shader
  glUseProgram(gpu.shader_program->GetProgram());

  for (QMap<QString, MayaGpuSet>::iterator i = instances.begin(); i != instances.end(); i++)
  {
    i->Render(gpu);
  }

  // Desactivation Shader (pour rendre Background, ...)
  glUseProgram(0);
}/*!
\brief Render the set of instances.
*/
void MayaGpuAll::RenderBBox(Camera camera, int width, int height)
{
  for (QMap<QString, MayaGpuSet>::iterator i = instances.begin(); i != instances.end(); i++)
  {
    i->RenderBBox(camera, width, height);
  }
}

/*!
\brief Clear all instances.
*/
void MayaGpuAll::Clear()
{
  for (QMap<QString, MayaGpuSet>::iterator i = instances.begin(); i != instances.end(); i++)
  {
    i->DeleteBuffers();
  }
  instances.clear();
}

/*!
\brief clear all frames, preserving the instances.
*/
void MayaGpuAll::ClearFrames()
{
  for (QMap<QString, MayaGpuSet>::iterator i = instances.begin(); i != instances.end(); i++)
  {
    i->ClearFrames();
  }
}

/*!
\brief Add an instance set to the scene.

Checks if the object already exists in the collection of models: if so,
it simply creates a new reference to it, otherwise it creates a new entry.
\param ins The instance.
*/
void MayaGpuAll::Append(const MayaGeometrySet& ins)
{
  // Check if static shape already exists
  if (instances.find(ins.GetName()) != instances.end())
  {
    for (int j = 0; j < ins.count(); j++)
    {
      instances[ins.GetName()].Append(ins.GetFrameScaled(j));
    }
    instances[ins.GetName()].RefreshFrames();
  }
  else
  {
    // Add static shape to the set of static objects and creates an instance
    instances[ins.GetName()] = MayaGpuSet(ins);
  }
}

/*!
\brief Append a set of instances with the existing one.
\param scene The set of instances.
*/
void MayaGpuAll::Append(const MayaGeometryAll& scene)
{
  for (QMap<QString, MayaGeometrySet>::const_iterator i = scene.instances.begin(); i != scene.instances.end(); i++)
  {
    Append(*i);
  }
}/*!
\brief Compute some statistics.

This function gathers some statistics and pack them into the structure.
*/
MayaStatistics MayaGpuAll::GetStatistics() const
{
  MayaStatistics s;
  for (QMap<QString, MayaGpuSet>::const_iterator i = instances.begin(); i != instances.end(); i++)
  {
    s += i->GetStatistics();
  }
  return s;
}