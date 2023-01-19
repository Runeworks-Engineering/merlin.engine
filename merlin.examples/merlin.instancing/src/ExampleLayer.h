#pragma once

#include <Merlin.h>

using namespace Merlin::Renderer;
using namespace Merlin::Tensor;
using namespace Merlin::Scene;

class ExampleLayer : public Merlin::Layer
{
public:
	ExampleLayer();
	virtual ~ExampleLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnEvent(Merlin::Event& event) override;
	virtual void OnUpdate(Merlin::Timestep ts) override;
	virtual void OnImGuiRender() override;

	void updateFPS(Merlin::Timestep ts);

private:
	GLsizei _width = 1080, _height = 720;
	
	//Mesh rendering
	std::shared_ptr<Shader> modelShader;
	std::shared_ptr<Shader> axisShader;
	std::shared_ptr<Mesh> axis;
	std::shared_ptr<Mesh> model;

	//Mesh instancing
	std::shared_ptr<ParticleSystem> particleSystem;
	std::shared_ptr<Shader> particleShader;
	std::shared_ptr<ComputeShader> physics;
	std::shared_ptr<ComputeShader> init;

	//Camera
	Shared<Camera> camera;
	Shared<CameraController> cameraController;

	bool paused = true;
	glm::vec3 model_matrix_translation = { 0.0f, 0.0f, 0.0f};
	float sim_speed = 1;
	float camera_speed = 1;
	float FPS = 0;
	float FPS_sample = 0;
};