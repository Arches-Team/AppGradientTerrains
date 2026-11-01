#version 430

layout(location = 0) in vec4 Position;

layout(location = 0) uniform mat4 ViewMatrix;
layout(location = 1) uniform mat4 ProjMatrix;
layout(location = 3) uniform mat4 ModelMatrix;

void main()
{
    gl_Position = ProjMatrix * ViewMatrix * ModelMatrix * Position;
}