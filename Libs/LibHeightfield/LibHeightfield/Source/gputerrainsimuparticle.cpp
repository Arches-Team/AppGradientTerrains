#include "gputerrainsimu.h"
#include "shuffle.h"
#include "cpu.h"

/*!
\class GPUHydraulicErosionParticle gputerrainsimu.h
\brief Hydraulic erosion simulation on the GPU.

The algorithm uses a particle-based approach all simulated independently on the terrain.

The implementation is based on https://github.com/SebLague/Hydraulic-Erosion
It has a race condition: multiple threads can be writing to the elevation buffer at the same time,
Thus making some operations not taken into account. The original shader suffers from the same issue.


AXEL : this doesn't work anymore, have to figure it out. Weird buffer issue ?
*/

/*!
\brief Constructor. Initialize everthing to zero.
*/
GPUHydraulicErosionParticle::GPUHydraulicErosionParticle()
{
  hardnessFunctionIndex = 0;

  totalBufferSize = dispatchSize = 0;
  nx = ny = 0;
  scaleZa = scaleZb = 0.0;
}

/*!
\brief Destructor. Destroys OpenGl buffers and shader program.
*/
GPUHydraulicErosionParticle::~GPUHydraulicErosionParticle()
{
  bedrockBuffer.Destroy();
  sedimentBuffer.Destroy();
  randomIndicesBuffer.Destroy();
  brushIndicesBuffer.Destroy();
  brushWeightsBuffer.Destroy();
  outHardnessBuffer.Destroy();
  hardnessBuffer.Destroy();
  precipitationBuffer.Destroy();

  simulationShader.Destroy();
}

/*!
\brief Initialize the erosion engine from a heightfield.
Must be called before any Step().
\param hf heightfield
*/
void GPUHydraulicErosionParticle::Init(const HeightField& hf)
{
  // nx/ny must be stored.
  nx = hf.GetSizeX();
  ny = hf.GetSizeY();
  totalBufferSize = hf.VertexSize();
  dispatchSize = 64;

  hf.GetRange(scaleZa, scaleZb);
  HeightField hfNormalized = hf;
  hfNormalized.Normalize();

  // Elevations, brushes, random indices
  std::vector<float> brushWeights, sediments;
  std::vector<int> brushIndexOffsets;
  tmpData.resize(totalBufferSize);
  sediments.resize(totalBufferSize);
  for (int i = 0; i < totalBufferSize; i++)
  {
    tmpData[i] = hfNormalized.at(i);
    sediments[i] = 0.0f;
  }

  QVector<int> tableTmp = Shuffle().Table(totalBufferSize);
  randomIndices = std::vector<int>(tableTmp.begin(), tableTmp.end());

  float weightSum = 0;
  int erosionBrushRadius = 3;
  for (int brushY = -erosionBrushRadius; brushY <= erosionBrushRadius; brushY++)
  {
    for (int brushX = -erosionBrushRadius; brushX <= erosionBrushRadius; brushX++)
    {
      float sqrDst = brushX * brushX + brushY * brushY;
      if (sqrDst < erosionBrushRadius * erosionBrushRadius)
      {
        brushIndexOffsets.push_back(brushY * nx + brushX);
        float brushWeight = 1 - sqrt(sqrDst) / erosionBrushRadius;
        weightSum += brushWeight;
        brushWeights.push_back(brushWeight);
      }
    }
  }
  for (int i = 0; i < brushWeights.size(); i++)
    brushWeights[i] /= weightSum;

  // Buffers and buffers data
  QString fullPath = System::GetResource("ARCHESLIBDIR", "/LibHeightfield/LibHeightfield/Shaders/heightfield_hydraulic_particle.glsl");
  if (fullPath.isEmpty())
  {
    std::cout << "GPUHydraulicErosionParticle::Init() : variable d'environnement ARCHESLIBDIR non d�finie" << std::endl;
    std::cin.get();
    exit(-1);
  }
  QByteArray ba = fullPath.toLocal8Bit();
  simulationShader.Initialize(ba.data());

  // Bedrock
  bedrockBuffer.Generate();
  bedrockBuffer.SetData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * totalBufferSize, &tmpData.front(), GL_STREAM_READ);

  sedimentBuffer.Generate();
  sedimentBuffer.SetData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * totalBufferSize, &sediments.front(), GL_STREAM_READ);

  brushIndicesBuffer.Generate();
  brushIndicesBuffer.SetData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * int(brushIndexOffsets.size()), &brushIndexOffsets.front(), GL_STREAM_READ);

  brushWeightsBuffer.Generate();
  brushWeightsBuffer.SetData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * int(brushWeights.size()), &brushWeights.front(), GL_STREAM_READ);

  randomIndicesBuffer.Generate();
  randomIndicesBuffer.SetData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * int(randomIndices.size()), &randomIndices.front(), GL_STREAM_READ);

  outHardnessBuffer.Generate();
  outHardnessBuffer.SetData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * totalBufferSize, 0, GL_STREAM_READ);

  hardnessBuffer.Generate();
  hardnessBuffer.SetData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * totalBufferSize, &sediments.front(), GL_STREAM_READ);

  precipitationBuffer.Generate();
  precipitationBuffer.SetData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * totalBufferSize, &sediments.front(), GL_STREAM_READ);

  precipitationBuffer.Generate();
  precipitationBuffer.SetData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * totalBufferSize, &sediments.front(), GL_STREAM_READ);

  // Uniforms
  simulationShader.Bind();
  simulationShader.SetUniform("nx", nx);
  simulationShader.SetUniform("ny", ny);
  simulationShader.SetUniform("brushLength", int(brushIndexOffsets.size()));
  simulationShader.SetUniform("borderSize", erosionBrushRadius);
  simulationShader.Unbind();
}

/*!
\brief Performs N step of hydraulic erosion on the underlying heightfield.
\param n step count
*/
void GPUHydraulicErosionParticle::Step(int n)
{
  simulationShader.Bind();

  bedrockBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 0);
  sedimentBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 1);
  brushIndicesBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 2);
  brushWeightsBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 3);
  randomIndicesBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 4);
  hardnessBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 5);
  precipitationBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 6);
  outHardnessBuffer.BindAt(GL_SHADER_STORAGE_BUFFER, 7);

  for (int i = 0; i < n; i++)
  {
    // Hardness/Precipitation flag
    simulationShader.SetUniform("useHardness", int(hardnessBuffer.used));
    simulationShader.SetUniform("hardnessFunctionIndex", hardnessFunctionIndex);
    simulationShader.SetUniform("usePrecipitation", int(precipitationBuffer.used));

    // Random indices for the step
    int start = Random::R239.Integer(totalBufferSize - 64);
    simulationShader.SetUniform("startIndex", start);

    // Dispatch
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  }
  glUseProgram(0);
}/*!
\brief Reads back newest bedrock and sediment elevation from the GPU, into initialized scalarfields.
*/
void GPUHydraulicErosionParticle::GetData(ScalarField2& hf)
{
  bedrockBuffer.GetData(tmpData);
  for (int i = 0; i < totalBufferSize; i++)
    hf[i] = double(tmpData[i]);

  sedimentBuffer.GetData(tmpData);
  for (int i = 0; i < totalBufferSize; i++)
    hf[i] += double(tmpData[i]);

  double c = (scaleZb - scaleZa) / (1.0 - 0.0);
  for (int i = 0; i < totalBufferSize; i++)
    hf[i] = scaleZa + c * hf[i];
}