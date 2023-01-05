#version 460

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

struct Particle {
  vec3 position;
  vec3 velocity;
  float density;
  float pressure;
};

layout (std430, binding = 1) buffer ParticleBuffer {
  Particle particles[];
}Pbuffer;

uniform uint grid;
uniform float gridSpacing;

void main() {
  uint index = gl_GlobalInvocationID.x;
  // Calculate the indices for each dimension using div and mod
  uint i = index / (grid * grid);
  uint j = (index / grid) % grid;
  uint k = index % grid;

  Particle pt;

  pt.position = vec3(i, j, k) * gridSpacing;
  pt.velocity = vec3(i,j,k) * 0.0f;
  pt.pressure = 0.0f;
  pt.density = 0.0f;

  Pbuffer.particles[index] = pt;
}