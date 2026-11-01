#pragma once
#ifdef _MSC_VER
#include "glew.h"
#else
#include "GL/glew.h"
#endif
#include "heightfield.h"

class SimpleGeometricMultigridFloat : public ScalarField2 {
public:
    SimpleGeometricMultigridFloat(const ScalarField2& alpha,
        const ScalarField2& altitude, const ScalarField2& laplacian);
    ~SimpleGeometricMultigridFloat();
    SimpleGeometricMultigridFloat(const SimpleGeometricMultigridFloat&) = delete;
    SimpleGeometricMultigridFloat& operator=(const SimpleGeometricMultigridFloat&) = delete;
    void InitGL();
    void Solve();
    void VCycle(int);
    ScalarField2 GetResult();
    FloatArray* alpha = nullptr;         //!< alpha coefficient
    FloatArray* altitude = nullptr;      //!< altitude constraint 
    FloatArray* bufferA = nullptr;       //!< First buffer
    FloatArray* bufferB = nullptr;       //!< Second buffer
    FloatArray* laplacian = nullptr;     //!< Laplacian field
    int mgsize;
    int minsize;
    int nrec;
    double trec;
protected:
    Box2 box;
    static const unsigned int WORK_GROUP_SIZE_X = 32;
    static const unsigned int WORK_GROUP_SIZE_Y = 32;
    unsigned int  bufferElems;
    GLuint shaderStepAtoB = 0;
    GLuint* glbufferAlpha = nullptr;
    GLuint* glbufferAltitude = nullptr;
    GLuint* glbufferA = nullptr;
    GLuint* glbufferB = nullptr;
    GLuint* glbufferLaplacian = nullptr;
};
