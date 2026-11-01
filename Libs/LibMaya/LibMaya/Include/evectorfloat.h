// Maya  

#pragma once

#include "matrix.h"

class VectorFloat
{
public:
  float x = 0.0f, y = 0.0f, z = 0.0f; //!< Components.
public:
    explicit VectorFloat(const Vector&);
  };


//! Create a vector.
inline VectorFloat::VectorFloat(const Vector& v) :x(v[0]), y(v[1]), z(v[2])
{}













