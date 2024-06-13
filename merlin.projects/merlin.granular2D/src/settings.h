#pragma once
#include <Merlin.h>
#include <glm/glm.hpp>



struct Particle {
	glm::vec2 position;
	glm::vec2 pposition;
	glm::vec2 velocity;
	float density;
	float lambda;
	GLuint phase;
	GLuint binIndex;
	GLuint id;
	GLuint padding;
};

struct Bin {
	GLuint count; //particle count in the bin
	GLuint sum;   //global sum
	GLuint csum;  //local sum
	GLuint index; //bin index
};


struct Settings {
	const float particleRadius = 0.25;
	const float smoothingRadius = 5 * particleRadius;
	const float bWidth = smoothingRadius;


	//Boundary Volume dimensions
	glm::vec2 bb = glm::vec2(400, 400);

	// Physics Parameters
	Uniform<float> timestep							= Uniform<float>("dt", 0.0016);
	Uniform<float> restDensity						= Uniform<float>("restDensity", 1.0);
	Uniform<float> particleMass						= Uniform<float>("particleMass", smoothingRadius);
	Uniform<float> artificialViscosityMultiplier	= Uniform<float>("artificialViscosityMultiplier", 0.5);
	Uniform<float> artificialPressureMultiplier		= Uniform<float>("artificialPressureMultiplier", 2.0);

	//Elastic solids
	float stiffness = 0.0;

	//GPU Threading settings
	GLuint pThread = 1000000; //Max Number of particles (thread) (10 milion)

	//Solver settings
	int solver_substep = 10;
	int solver_iteration = 10;
	float overRelaxation = 1.0;

	//calulated
	GLuint pWkgSize = 512; //Number of thread per workgroup
	GLuint bWkgSize = 128; //Number of thread per workgroup

	GLuint pWkgCount = (pThread + pWkgSize - 1) / pWkgSize; //Total number of workgroup needed
	GLuint bThread = int(bb.x / (bWidth)) * int(bb.y / (bWidth)); //Total number of bin (thread)
	GLuint blockSize = floor(log2f(bThread));
	GLuint blocks = (bThread + blockSize - 1) / blockSize;
	GLuint bWkgCount = (blocks + bWkgSize - 1) / bWkgSize; //Total number of workgroup needed
};
