#pragma once

#include "heightfield.h"

class HeightFieldShader
{
protected:
  HeightField owned; //!< Copy of the heightfield, only used when the shader is built from a temporary.
  const HeightField& heightfield; //!< Shaded heightfield: either the referenced heightfield or the owned copy.
public:
  explicit HeightFieldShader(const HeightField&);
  explicit HeightFieldShader(HeightField&&);
  //! The reference may designate the owned copy of the source shader.
  HeightFieldShader(const HeightFieldShader&) = delete;
  HeightFieldShader& operator=(const HeightFieldShader&) = delete;
  //! Empty
  ~HeightFieldShader();

  QImage Relief(bool = false) const;
    
    
          
  
  
};


