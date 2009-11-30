#version 120 
#extension GL_EXT_geometry_shader4 : enable

// There are multiple OpenGL geometry shader specifications.  This shader is written against GLSL 1.20
// with the geometry shader extension, which supports a wide variety of GPUs.

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

varying in vec3 normal[3];

void main() {
	for (int i = 0; i < 3; ++i){
		gl_Position = projectionMatrix * modelViewMatrix  * (gl_PositionIn[i] + vec4(normal[i] * -0.1, 0.0));
		EmitVertex();
	}
	EndPrimitive();
}
