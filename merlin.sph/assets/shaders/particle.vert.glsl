#version 460 core

struct Particle {
  vec4 position;
  vec4 velocity;
  float density;
  float pressure;
  float temperature;
  float conductivity;
};

layout (std430, binding = 1) buffer ParticleBuffer {
  Particle particles[];
};

layout (location = 0) in vec3 _position;

out vec3 position;
out vec3 offset;
out vec3 normal;
out vec3 color;
out vec2 texCoord;

uniform float radius;
uniform mat4 view;
uniform mat4 model;

void main() {
	offset = vec3(particles[gl_InstanceID].position);

	position = vec3(model * vec4((_position) * radius + offset, 1.0f) );
	color = vec3(0.6,0.6,1.0f);
	//color = normalize(offset)*vec3(1);
	//color = vec3(particles[gl_InstanceID].pressur);
	gl_Position = view * vec4(position, 1.0f);
}