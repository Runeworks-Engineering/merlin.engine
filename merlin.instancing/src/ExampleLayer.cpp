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

	init = std::make_shared<ComputeShader>("init");
	init->Compile("assets/shaders/particle.init.glsl");

	//Load models
	axis = ModelLoader::LoadAxis("axis");
	model = ModelLoader::LoadPlane("plane");
	//model->LoadTexture("assets/textures/wall.jpg", Texture::Type::DIFFUSE, GL_RGB);
	model->translate(glm::vec3(0, 0, -1));

	//Particle System settings
	GLsizeiptr gridSize = 20;
	GLsizeiptr partCount = gridSize * gridSize * gridSize;
	float gridWidth = 2.0f;

	//Create particle system
	particleSystem = CreateShared<ParticleSystem>("ParticleSystem", partCount);

	//Define the mesh for instancing (Here a cube)
	Shared<Primitive> cube = Primitive::CreateSphere(1.0f, 20, 20);//Primitive::CreateCube(1.0f);
	//cube->SetDrawMode(GL_LINES);
	particleSystem->SetPrimitive(cube);

	//Create the buffer
	Shared<SSBO> buffer = CreateShared<SSBO>("ParticleBuffer");
	buffer->SetBindingPoint(1);
	buffer->Allocate<DefaultParticle>(partCount);
	particleSystem->AddStorageBuffer(buffer);
	particleSystem->AddComputeShader(physics);
	
	physics->Use();
	physics->SetUInt("grid", gridSize);
	physics->SetFloat("gridSpacing", gridWidth/float(gridSize));

	init->Use();
	init->SetUInt("grid", gridSize);
	init->SetFloat("gridSpacing", gridWidth / float(gridSize));
	particleSystem->Execute(init);

	float smoothingRadius = 0.005f * 4;
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
	

	particleShader->Use();
	particleShader->SetUniform3f("lightPos", glm::vec3(0, 0, 0));
	particleShader->SetUniform3f("lightColor", glm::vec3(0.3, 0.3, 0.3));
	particleShader->SetUniform3f("viewPos", cameraController.GetCamera().GetPosition());
	particleShader->SetFloat("shininess", 4.0f);

	model->Draw(modelShader, cameraController.GetCamera().GetViewProjectionMatrix());
	axis->Draw(axisShader, cameraController.GetCamera().GetViewProjectionMatrix());

	physics->Use();
	physics->SetFloat("speed", sim_speed);


	if(!paused) particleSystem->Update(ts);
	particleSystem->Draw(particleShader, cameraController.GetCamera().GetViewProjectionMatrix());

}

void ExampleLayer::OnImGuiRender()
{
	ImGui::Begin("Infos");

	model_matrix_translation = cameraController.GetCamera().GetPosition();
	camera_speed = cameraController.GetCameraSpeed();

	if (FPS_sample > 0) {
		ImGui::LabelText("FPS", std::to_string(1.0f / (FPS / FPS_sample)).c_str());
		if (FPS_sample > 50) FPS_sample = 0;
	}

	if (paused) {
		if (ImGui::ArrowButton("Run simulation", 1))
			paused = !paused;
	}
	else {
		if (ImGui::SmallButton("Pause simulation"))
			paused = !paused;
	}

	if (ImGui::SmallButton("Reset simulation"))
		particleSystem->Execute(init); //init position using init compute shader

	if (ImGui::DragFloat3("Camera position", &model_matrix_translation.x, -100.0f, 100.0f)) {
		cameraController.GetCamera().SetPosition(model_matrix_translation);
	}
	if (ImGui::SliderFloat("Camera speed", &camera_speed, 0.0, 100.0f)) {
		cameraController.SetCameraSpeed(camera_speed);
	}
	if (ImGui::SliderFloat("Simulation speed", &sim_speed, 0.0, 20.0f)) {
	}


	ImGui::End();
}
