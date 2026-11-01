#include "diffusionterrain.h"
#include "gpu-shader.h"
#include "mathematics.h"
#include "QtCore/QElapsedTimer"
#include <stdexcept>

/*!
\brief The GPU buffers, dispatches and restriction assume square grids (s x s at every level).
*/
static void CheckSquareGrid(int nx, int ny)
{
  if (nx != ny)
    throw std::invalid_argument("Multigrid solver requires a square grid, got " + std::to_string(nx) + "x" + std::to_string(ny));
}

/*!
\brief Delete an array of GL buffers allocated with new[] and glGenBuffers. Requires a current OpenGL context.
*/
static void DeleteGLBuffers(int n, GLuint*& buffers)
{
  if (buffers != nullptr)
  {
    glDeleteBuffers(n, buffers);
    delete[] buffers;
    buffers = nullptr;
  }
}

/////////////////////////////// SimpleGeometricMultigrid - float version


SimpleGeometricMultigridFloat::SimpleGeometricMultigridFloat(const ScalarField2& alph, const ScalarField2& alt, const ScalarField2& lap)
  : ScalarField2(alt) {
  CheckSquareGrid(nx, ny);
  bufferElems = nx * ny;
  int s = nx;
  box = alt.GetBox();

  mgsize = 1;
  minsize = 9;
  nrec = 0;
  trec = .0;
  while (s > minsize) {
    mgsize++;
    s = s / 2 + 1;
  }
  std::cout << "# of resolutions " << mgsize <<std::endl;
  altitude = new FloatArray[mgsize];
  alpha = new FloatArray[mgsize];

  bufferA = new FloatArray[mgsize];
  bufferB = new FloatArray[mgsize];
  laplacian = new FloatArray[mgsize];
  s = nx / 2 + 1;
  altitude[0] = FloatArray(nx * ny);
  alpha[0] = FloatArray(nx * ny);

  bufferA[0] = FloatArray(nx * ny);
  bufferB[0] = FloatArray(nx * ny);
  laplacian[0] = FloatArray(nx * ny);

  for (int i = 0; i < nx; i++) {
    for (int j = 0; j < ny; j++) {
      altitude[0][i * ny + j] = alt.at(i, j);
      alpha[0][i * ny + j] = alph.at(i, j);
      laplacian[0][i * ny + j] = lap.at(i, j);
    }
  }

  int olds = nx;
  // geometric multigrid -> find the coarse system depending on the geometric fine system
  for (int r = 1; r < mgsize; r++) {
    altitude[r] = FloatArray(s * s);
    alpha[r] = FloatArray(s * s); // laplacian coef

    bufferA[r] = FloatArray(s * s);
    bufferB[r] = FloatArray(s * s);
    laplacian[r] = FloatArray(s * s);
    for (int i = 0; i < s; i++)
    {
      for (int j = 0; j < s; j++)
      {
        int ii, jj;
        int iimin = -1;
        int jjmin = -1;
        int iimax = 1;
        int jjmax = 1;
        if (i == 0)
          iimin = 0;
        else if (i == s - 1)
          iimax = 0;
        if (j == 0)
          jjmin = 0;
        else if (j == s - 1)
          jjmax = 0;
        bool fixed = false;
        double maltitude = .0;
        double nfixed = .0;
        bool gradient = false;
        Vector2 dirgrad;
        double maxweightgrad = .0;
        bool lap = false;
        double sumlap = .0;
        double nlap = .0;
        double newalpha = 0.0;
        double sumcoef = .0;
        for (ii = iimin; ii <= iimax; ii++)
        {
          for (jj = jjmin; jj <= jjmax; jj++)
          {
            int iii = 2 * i + ii;
            int jjj = 2 * j + jj;
            if (iii >= olds || jjj >= olds)
              continue;
            double coef = 1.0 / double((1 << abs(ii)) * (1 << abs(jj)));
            newalpha += coef * alpha[r - 1][iii * olds + jjj];
            sumcoef += coef;
            if (alpha[r - 1][iii * olds + jjj] < 1.0) // there is a fixed altitude constraint
            {
              fixed = true;

              maltitude += coef * (1.0 - alpha[r - 1][iii * olds + jjj]) * altitude[r - 1][iii * olds + jjj];
              nfixed += coef * (1.0 - alpha[r - 1][iii * olds + jjj]);
            }

            if (alpha[r - 1][iii * olds + jjj] > .0) // there is a laplacian constraint here
            {
              lap = true;
              sumlap += coef * alpha[r - 1][iii * olds + jjj] * laplacian[r - 1][iii * olds + jjj]; // not an average, a geometric-weighted sum
              nlap += coef * alpha[r - 1][iii * olds + jjj];
            }
          }
        }

        if (fixed) {
          alpha[r][i * s + j] = 0.0;

          altitude[r][i * s + j] = maltitude / nfixed; // geometric-weighted average if several cells were concerned
        }
        else { // only laplacian

          alpha[r][i * s + j] = 1.0;
          laplacian[r][i * s + j] = sumlap;// / nlap * 4.0;
        }
      }
    }
    s = s / 2 + 1;
    olds = olds / 2 + 1;
  }
}

SimpleGeometricMultigridFloat::~SimpleGeometricMultigridFloat()
{
  DeleteGLBuffers(mgsize, glbufferAlpha);
  DeleteGLBuffers(mgsize, glbufferAltitude);
  DeleteGLBuffers(mgsize, glbufferA);
  DeleteGLBuffers(mgsize, glbufferB);
  DeleteGLBuffers(mgsize, glbufferLaplacian);
  if (shaderStepAtoB != 0) glDeleteProgram(shaderStepAtoB);

  delete[] alpha;
  delete[] altitude;
  delete[] bufferA;
  delete[] bufferB;
  delete[] laplacian;
}

void SimpleGeometricMultigridFloat::InitGL()
{
  // load shader
  std::string definitions = "";
  definitions += "#define WORK_GROUP_SIZE_X " + std::to_string(WORK_GROUP_SIZE_X) + "\n";
  definitions += "#define WORK_GROUP_SIZE_Y " + std::to_string(WORK_GROUP_SIZE_Y) + "\n";
  shaderStepAtoB = read_program("./Shaders/mgstepfloat.glsl", definitions.c_str());
  std::cerr << "Compute shader loaded!" << std::endl;

  // create buffers
  int GPUsize = 0;
  glbufferAlpha = new GLuint[mgsize];
  glGenBuffers(mgsize, glbufferAlpha);
  int s = nx;
  for (int r = 0; r < mgsize; r++) {
    int nelems = s * s;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, glbufferAlpha[r]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, nelems * sizeof(float), (const void*)(&(alpha[r][0])), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    GPUsize += nelems * sizeof(float);
    s = s / 2 + 1;
  }

  glbufferAltitude = new GLuint[mgsize];
  glGenBuffers(mgsize, glbufferAltitude);
  s = nx;
  for (int r = 0; r < mgsize; r++) {
    int nelems = s * s;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, glbufferAltitude[r]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, nelems * sizeof(float), (const void*)(&(altitude[r][0])), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    GPUsize += nelems * sizeof(float);
    s = s / 2 + 1;
  }



  glbufferA = new GLuint[mgsize];
  glGenBuffers(mgsize, glbufferA);
  s = nx;
  for (int r = 0; r < mgsize; r++) {
    int nelems = s * s;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, glbufferA[r]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, nelems * sizeof(float), (const void*)(&(bufferA[r][0])), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    GPUsize += nelems * sizeof(float);
    s = s / 2 + 1;
  }

  glbufferB = new GLuint[mgsize];
  glGenBuffers(mgsize, glbufferB);
  s = nx;
  for (int r = 0; r < mgsize; r++) {
    int nelems = s * s;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, glbufferB[r]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, nelems * sizeof(float), (const void*)(&(bufferB[r][0])), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    GPUsize += nelems * sizeof(float);
    s = s / 2 + 1;
  }

  glbufferLaplacian = new GLuint[mgsize];
  glGenBuffers(mgsize, glbufferLaplacian);
  s = nx;
  for (int r = 0; r < mgsize; r++) {
    int nelems = s * s;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, glbufferLaplacian[r]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, nelems * sizeof(float), (const void*)(&(laplacian[r][0])), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    GPUsize += nelems * sizeof(float);
    s = s / 2 + 1;
  }
  std::cout << "GPU buffers size en bytes " << GPUsize <<std::endl;

}

void SimpleGeometricMultigridFloat::Solve() {
  QElapsedTimer chrono;
  chrono.start();
  VCycle(0);
  std::cout << "Multigrid time: " << chrono.elapsed() << " ms" <<std::endl;
  nrec++;
  trec += chrono.elapsed();
}


void SimpleGeometricMultigridFloat::VCycle(int level) {
  int s = nx;
  for (int i = 0; i < level; i++) {
    s = s / 2 + 1;
  }
  int nelem = s * s;

  int nit = 50 + (10 * (mgsize - level));
  std::cout << "level " << level << " " << nit << " iterations" <<std::endl;

  //laplacian[level].CreateImage(AnalyticPalette(3)).save(QString("mg") + QString::number(level) + QString("laplacian.png"));

  if (level == mgsize - 1) {

    // First : smooth (Jacobi iterations should not need too much of these)
    for (int step = 0; step < nit; step++) {

      glUseProgram(shaderStepAtoB);

      glProgramUniform1i(shaderStepAtoB, glGetUniformLocation(shaderStepAtoB, "GridSizeX"), s); // attention X,Y
      glProgramUniform1i(shaderStepAtoB, glGetUniformLocation(shaderStepAtoB, "GridSizeY"), s);

      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, glbufferAlpha[level]);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, glbufferAltitude[level]);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, glbufferA[level]);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, glbufferB[level]);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, glbufferLaplacian[level]);


      glDispatchCompute((s / WORK_GROUP_SIZE_X) + 1, (s / WORK_GROUP_SIZE_Y) + 1, 1); // attention X,Y
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, 0);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, 0);

      glUseProgram(0);
      std::swap(glbufferA[level], glbufferB[level]);
    }
    // get the iteration result in bufferA (swap has just been performed before stopping the loop)

    glGetNamedBufferSubData(glbufferA[level], 0, sizeof(float) * nelem, &(bufferA[level][0]));

    //bufferA[level].CreateImage().save(QString("mg") + QString::number(level) + QString("smooth.png"));

    return;
  }


  // we do not need to compute the residual here, because it's not yet a complete multi-grid method, i.e. we do not perform the next
  // iteration on the residual/error but on the restriction (bilinear) of the result

  // restriction operator -> bilinear interpolation using geometric weights



  //bufferA[level + 1].CreateImage().save(QString("mg") + QString::number(level) + QString("bufferAbeforerec.png"));
  // solve the next level - reccursive call //////////////////////////////////////////////////////////
  VCycle(level + 1);

  //bufferA[level + 1].CreateImage().save(QString("mg") + QString::number(level) + QString("beforesmoothing.png"));



  // prolongation operator : computes the fine (level) interpolation wrt the coarse level result (level+1)
  for (int i = 0; i < s; i++) {
    for (int j = 0; j < s; j++) {
      double val = .0;
      if (i % 2 == 0 && j % 2 == 0) { // both even row and column
        val = bufferA[level + 1][(i / 2) * (s / 2 + 1) + j / 2];
      }
      else if (i % 2 == 0 && j % 2 == 1) { // even column and odd row
        val = 0.5 * bufferA[level + 1][(i / 2) * (s / 2 + 1) + j / 2] + 0.5 * bufferA[level + 1][(i / 2) * (s / 2 + 1) + j / 2 + 1];
      }
      else if (i % 2 == 1 && j % 2 == 0) { // even row and odd column
        val = 0.5 * bufferA[level + 1][(i / 2) * (s / 2 + 1) + j / 2] + 0.5 * bufferA[level + 1][(i / 2 + 1) * (s / 2 + 1) + j / 2];
      }
      else { // odd column and row
        val = 0.25 * bufferA[level + 1][(i / 2) * (s / 2 + 1) + j / 2] + 0.25 * bufferA[level + 1][(i / 2) * (s / 2 + 1) + j / 2 + 1]
          + 0.25 * bufferA[level + 1][(i / 2 + 1) * (s / 2 + 1) + j / 2] + 0.25 * bufferA[level + 1][(i / 2 + 1) * (s / 2 + 1) + j / 2 + 1];
      }
      bufferA[level][i * s + j] = val;
    }
  }

  // last step : iterate to refine the result on the current level
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, glbufferA[level]);
  glBufferData(GL_SHADER_STORAGE_BUFFER, nelem * sizeof(float), (const void*)(&(bufferA[level][0])), GL_DYNAMIC_DRAW);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  // First : smooth (Jacobi iterations should not need too much of these)

  for (int step = 0; step < nit; step++)
  {


    glUseProgram(shaderStepAtoB);

    glProgramUniform1i(shaderStepAtoB, glGetUniformLocation(shaderStepAtoB, "GridSizeX"), s); // attention X,Y
    glProgramUniform1i(shaderStepAtoB, glGetUniformLocation(shaderStepAtoB, "GridSizeY"), s);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, glbufferAlpha[level]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, glbufferAltitude[level]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, glbufferA[level]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, glbufferB[level]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, glbufferLaplacian[level]);



    glDispatchCompute((s / WORK_GROUP_SIZE_X) + 1, (s / WORK_GROUP_SIZE_Y) + 1, 1); // attention X,Y
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, 0);


    glUseProgram(0);

    std::swap(glbufferA[level], glbufferB[level]);
  }
  // get the iteration result in bufferA (swap has just been performed before stopping the loop)
  glGetNamedBufferSubData(glbufferA[level], 0, sizeof(float)* nelem, &(bufferA[level][0]));
  //bufferA[level].CreateImage().save(QString("mg") + QString::number(level) + QString("final.png"));
}



ScalarField2 SimpleGeometricMultigridFloat::GetResult() {

  glUseProgram(shaderStepAtoB);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, glbufferA[0]); // note we have the most recent buffer here due to swap!
  void* ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferElems * sizeof(float), GL_MAP_READ_BIT);
  memcpy(&(bufferA[0][0]), ptr, bufferElems * sizeof(float));
  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  glUseProgram(0);

  ScalarField2 result(box, nx, ny);
  FloatArray farray(bufferA[0]);
  for (int i = 0; i < nx; i++) {
    for (int j = 0; j < ny; j++) {
      result(i, j) = farray[i * ny + j];
    }
  }
  return result;
}

