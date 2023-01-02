#include "ExampleLayer.h"

using namespace Merlin;
using namespace Merlin::Utils;
using namespace Merlin::Renderer;

#include <iostream>
#include <iomanip>

ExampleLayer::ExampleLayer() : cameraController(45.0f, 16.0f / 9.0f){
	cameraController.GetCamera().SetPosition(glm::vec3(-2.0f, 0.0f, 1.0f));
}

ExampleLayer::~ExampleLayer(){

}

void ExampleLayer::OnAttach(){
	EnableGLDebugging();
	Window* wd = &Application::Get().GetWindow();
	_height = wd->GetHeight();
	_width = wd->GetWidth();

	Console::SetLevel(ConsoleLevel::_INFO);

	// Init OpenGL stuff
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glFrontFace(GL_CW);

	//Shaders
	axisShader = std::make_shared<Shader>("axis");
	axisShader->Compile(
		"assets/shaders/axis.vert.glsl",
		"assets/shaders/axis.frag.glsl"
	);

	modelShader = std::make_shared<Shader>("model");
	modelShader->Compile(
		"assets/shaders/model.vert.glsl",
		"assets/shaders/model.frag.glsl"
	);

	particleShader = std::make_shared<Shader>("particle");
	particleShader->Compile(
		"assets/shaders/particle.vert.glsl",
		"assets/shaders/particle.frag.glsl"
	);
	
	//Compute Shaders
	physics = std::make_shared<ComputeShader>("physics");
	physics->Compile("assets/shaders/particle.update.glsl");

	std::shared_ptr<ComputeShader> init;
	init = std::make_shared<ComputeShader>("init");
	init->Compile("assets/shaders/particle.init.glsl");

	//Load models
	axis = ModelLoader::LoadAxis("axis");
	model = ModelLoader::LoadPlane("plane");
	model->translate(glm::vec3(0, 0, -1));


	//Particle System settings
	GLsizeiptr gridSize = 100;
	GLsizeiptr partCount = gridSize * gridSize * gridSize;
	float gridWidth = 2.0f;

	//Create particle system
	particleSystem = CreateShared<ParticleSystem>("ParticleSystem", partCount);
	

	//Define the mesh for instancing (Here a cube)
	std::vector<Vertex> vertices = {
		//   Coordinates
		 Vertex{ glm::vec3(-1, -1,  1)},   //        7--------6
		 Vertex{ glm::vec3( 1, -1,  1)},   //       /|       /|
		 Vertex{ glm::vec3( 1, -1, -1)},   //      4--------5 |
		 Vertex{ glm::vec3(-1, -1, -1)},   //      | |      | |
		 Vertex{ glm::vec3(-1,  1,  1)},   //      | 3------|-2
		 Vertex{ glm::vec3( 1,  1,  1)},   //      |/       |/
		 Vertex{ glm::vec3( 1,  1, -1)},   //      0--------1
		 Vertex{ glm::vec3(-1,  1, -1)}
	};
	std::vector<GLuint> indices = {
		1, 2, 6,// Right
		6, 5, 1,
		0, 4, 7,// Left
		7, 3, 0,
		4, 5, 6,// Top
		6, 7, 4,
		0, 3, 2,// Bottom
		2, 1, 0,
		0, 1, 5,// Back
		5, 4, 0,
		3, 7, 6,// Front
		6, 2, 3
	};

	particleSystem->AddComputeShader(physics);
	
	init->Use();
	init->SetUInt("grid", gridSize);
	init->SetFloat("gridSpacing", gridWidth/float(gridSize));
	particleSystem->Execute(init); //init position using init compute shader

	particleShader->Use();
	particleShader->SetFloat("radius", 0.5f * gridWidth / float(gridSize)); //Set particle radius

}

void ExampleLayer::OnDetach(){}

void ExampleLayer::updateFPS(Timestep ts) {
	if (FPS_sample == 0) {
		FPS = ts;
	}else {
		FPS += ts;
	}
	FPS_sample++;
}

void ExampleLayer::OnEvent(Event& event){
	cameraController.OnEvent(event);
}

void ExampleLayer::OnUpdate(Timestep ts){
	
	updateFPS(ts);
	cameraController.OnUpdate(ts);

	// Specify the color of the background
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	// Clean the back buffer and depth buffer (clean the screen)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	modelShader->Use();
	modelShader->SetUniform3f("lightPos", glm::vec3(0,0,0));
	modelShader->SetUniform3f("lightColor", glm::vec3(0.3, 0.3, 0.3));
	modelShader->SetUniform3f("viewPos", cameraController.GetCamera().GetPosition());
	modelShader->SetFloat("shininess", 16.0f);

	model->Draw(modelShader, cameraController.GetCamera().GetViewProjectionMatrix());
	axis->Draw(axisShader, cameraController.GetCamera().GetViewProjectionMatrix());

	particleSystem->Update(ts);
	particleSystem->Draw(particleShader, cameraController.GetCamera().GetViewProjectionMatrix());
}

void ExampleLayer::OnImGuiRender()
{
	ImGui::Begin("Infos");

	model_matrix_translation = cameraController.GetCamera().GetPosition();
	camera_speed = cameraController.GetCameraSpeed();

	if (FPS_sample > 0) {
		ImGui::LabelText("FPS", std::to_string(1.0f / (FPS/FPS_sample)).c_str());
		if(FPS_sample > 50) FPS_sample = 0;
	}
	
	if (ImGui::DragFloat3("Camera position", &model_matrix_translation.x, -100.0f, 100.0f)) {
		cameraController.GetCamera().SetPosition(model_matrix_translation);
	}
	if (ImGui::DragFloat("Camera speed", &camera_speed, 0.0, 100.0f)) {
		cameraController.SetCameraSpeed(camera_speed);
	}
	ImGui::End();
}
