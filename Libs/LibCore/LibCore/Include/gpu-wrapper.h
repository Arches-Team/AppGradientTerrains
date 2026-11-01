#pragma once

#include "gpu-shader.h"
#include "color.h"

class GLBuffer
{
protected:
  GLuint buffer;
public:
  inline GLBuffer() : buffer(0) { }
    
  void Generate();
  void Destroy();

  void SetData(int bufferType, size_t size, void* data, int hint = GL_STATIC_DRAW);
    void Bind(int bufferType) const;
  void BindAt(int bufferType, int bindIndex) const;
  
      template<typename T> void GetData(std::vector<T>& data) const
  {
    glGetNamedBufferSubData(buffer, 0, sizeof(T) * data.size(), data.data());
  }
};
inline void GLBuffer::Generate() {
  if (buffer == 0)
    glGenBuffers(1, &buffer);
}

inline void GLBuffer::Destroy() {
  glDeleteBuffers(1, &buffer);
  buffer = 0; // idempotent: mirrors the Generate() guard; prevents passing a stale non-zero handle on a second call
}

inline void GLBuffer::SetData(int bufferType, size_t size, void* data, int hint) {
  Bind(bufferType);
  glBufferData(bufferType, size, data, hint);
}



inline void GLBuffer::Bind(int bufferType) const {
  glBindBuffer(bufferType, buffer);
}

inline void GLBuffer::BindAt(int bufferType, int bindIndex) const
{
  glBindBufferBase(bufferType, bindIndex, buffer);
}




class GLShader
{
protected:
  GLuint program = 0;
public:
  inline GLShader() { }
  
  void Initialize(const char*);
      void Destroy();
  void Bind() const;
  void Unbind() const;

  // delete the undefined SetUniform (forces explicit parameters, avoid implicit conversions that break opengl)
  template<typename ...T1>
  void SetUniform(const char* name, T1...) const = delete;

        void SetUniform(const char*, int) const;
                  
                
  };

inline void GLShader::Initialize(const char* path) {
  program = read_program(path);
}



inline void GLShader::Destroy() {
  release_program(program);
}

inline void GLShader::Bind() const {
  glUseProgram(program);
}

inline void GLShader::Unbind() const {
  glUseProgram(0);
}






inline void GLShader::SetUniform(const char* name, int v) const {
  glUniform1i(glGetUniformLocation(program, name), v);
}















