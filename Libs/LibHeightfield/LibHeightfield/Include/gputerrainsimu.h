#pragma once

#include "gpu-wrapper.h"
#include "heightfield.h"

class GLOptionalStorageBuffer : public GLBuffer
{
public:
  bool used = false;
public:
  inline GLOptionalStorageBuffer() {}
  };


class GPUHydraulicErosionParticle
{
protected:
  GLShader simulationShader;		//!< Compute shader

  GLBuffer bedrockBuffer;			//!< Bedrock buffer
  GLBuffer sedimentBuffer;			//!< Sediment buffer
  GLBuffer randomIndicesBuffer;		//!< Random number buffer. Easier than generating random number on the GPU
  GLBuffer brushIndicesBuffer;		//!< Compute buffer
  GLBuffer brushWeightsBuffer;		//!< Compute buffer

  GLOptionalStorageBuffer hardnessBuffer;	//!< Optional Hardness buffer
  int hardnessFunctionIndex;
  GLBuffer outHardnessBuffer;		//!< Output hardness buffer (useful if we use the GPU function to generate hardness on the fly)
  GLOptionalStorageBuffer precipitationBuffer;	//!< Precipitation buffer

  int totalBufferSize;						//!< Total buffer size defined as nx * ny
  int dispatchSize;							//!< Single dispatch size
  int nx;									//!< Heightfield dimension
  int ny;									//!< Heightfield dimension

  double scaleZa;							//!< Heightfield min elevation
  double scaleZb;							//!< Heightfield max elevation
  std::vector<int> randomIndices;			//!< Random indices for the algorithm
  std::vector<float> tmpData;			//!< Temporary array for retreiving GPU data.

public:
  GPUHydraulicErosionParticle();
  ~GPUHydraulicErosionParticle();

  void Init(const HeightField& hf);
  void Step(int n);

        
          
  void GetData(ScalarField2& hf);
    };

