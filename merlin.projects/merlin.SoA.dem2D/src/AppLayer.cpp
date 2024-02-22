#include "AppLayer.h"

#include <GLFW/glfw3.h>
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace Merlin;

#define PROFILE(VAR, CODE) double start_ ## VAR ## _time = glfwGetTime(); CODE VAR = (glfwGetTime() - start_ ## VAR ## _time)*1000.0;
#define GPU_PROFILE(VAR, CODE) double start_ ## VAR ## _time = glfwGetTime(); CODE glFinish(); VAR = (glfwGetTime() - start_ ## VAR ## _time)*1000.0;

#define PROFILE_BEGIN(STARTVAR) STARTVAR = glfwGetTime();
#define PROFILE_END(STARTVAR, VAR) VAR = (glfwGetTime() - STARTVAR)*1000.0

AppLayer::AppLayer(){
	Window* w = &Application::Get().GetWindow();
	int height = w->GetHeight();
	int width = w->GetWidth();
	camera = CreateShared<Camera>(width, height, Projection::Orthographic);
	camera->setNearPlane(0.0f);
	camera->setFarPlane(10.0f);
	camera->Translate(glm::vec3(0, 0, 1));
	cameraController = CreateShared<CameraController2D>(camera);
	cameraController->SetZoomLevel(20);
	cameraController->SetCameraSpeed(100);
}

AppLayer::~AppLayer(){}

void AppLayer::OnAttach(){
	EnableGLDebugging();
	//ImGui::LoadIniSettingsFromDisk("imgui.ini");
	Console::SetLevel(ConsoleLevel::_TRACE);
	glfwSwapInterval(0);

	InitGraphics();
	InitPhysics();

	particleShader->Use();
	particleShader->Attach(*positionBuffer);
	particleShader->Attach(*predictedPositionBuffer);
	particleShader->Attach(*velocityBuffer);
	particleShader->Attach(*temperatureBuffer);
	particleShader->Attach(*metaBuffer);

	binShader->Use();
	binShader->Attach(*positionBuffer);
	binShader->Attach(*predictedPositionBuffer);
	binShader->Attach(*velocityBuffer);
	binShader->Attach(*temperatureBuffer);
	binShader->Attach(*metaBuffer);
	binShader->Attach(*binBuffer);

	ResetSimulation();
}

void AppLayer::OnDetach(){}

void AppLayer::OnEvent(Event& event){
	camera->OnEvent(event);
	cameraController->OnEvent(event);

	if (event.GetEventType() == EventType::MouseScrolled) {
		particleShader->Use();
		particleShader->SetFloat("zoomLevel", camera->GetZoom());
	}
}

float t = 0.0;

void AppLayer::OnUpdate(Timestep ts){
	cameraController->OnUpdate(ts);
	PROFILE_END(total_start_time, total_time);
	PROFILE_BEGIN(total_start_time);

	updateFPS(ts);

	GPU_PROFILE(render_time,
		renderer.Clear();
		renderer.RenderScene(scene, *camera);
	)

	if (!paused) {
		GPU_PROFILE(solver_total_time,
			Simulate(0.016);
		)
	}
}



void AppLayer::SyncUniforms() {

	solver->Use();

	//settings.timestep.Sync(*solver);
	settings.particleMass.Sync(*solver);
	settings.restDensity.Sync(*solver);
	solver->SetUInt("numParticles", numParticles);
	solver->SetFloat("dt", settings.timestep.value() / float(settings.solver_substep)); //Spawn particle after prediction
	solver->SetFloat("artificialViscosityMultiplier", settings.artificialViscosityMultiplier.value() * 0.01);
	solver->SetFloat("artificialPressureMultiplier", settings.artificialPressureMultiplier.value() * 0.01);

	particleShader->Use();
	particleShader->SetUInt("numParticles",numParticles);
	settings.restDensity.Sync(*particleShader);

	prefixSum->Use();
	prefixSum->SetUInt("dataSize", settings.bThread); //data size
	prefixSum->SetUInt("blockSize", settings.blockSize); //block size
}



void AppLayer::ApplyBufferSettings() {

	settings.pWkgCount = (settings.pThread + settings.pWkgSize - 1) / settings.pWkgSize; //Total number of workgroup needed
	settings.blockSize = floor(log2f(settings.bThread));
	settings.blocks = (settings.bThread + settings.blockSize - 1) / settings.blockSize;
	settings.bWkgCount = (settings.blocks + settings.bWkgSize - 1) / settings.bWkgSize; //Total number of workgroup needed

	solver->SetWorkgroupLayout(settings.pWkgCount);
	prefixSum->SetWorkgroupLayout(settings.bWkgCount);

	/*
	particleBuffer->Bind();
	particleBuffer->Resize(settings.pThread);
	particleBuffer->Unbind();

	particleCpyBuffer->Bind();
	particleCpyBuffer->Resize(settings.pThread);
	particleCpyBuffer->Unbind();

	sortedIndexBuffer->Bind();
	sortedIndexBuffer->Resize(settings.pThread);
	sortedIndexBuffer->Unbind();

	binBuffer->Bind();
	binBuffer->Resize(settings.bThread);
	binBuffer->Unbind();
	*/

	particleSystem->SetInstancesCount(settings.pThread);
	binSystem->SetInstancesCount(settings.bThread);
	
}

void AppLayer::InitGraphics() {
	// Init OpenGL stuff
	renderer.Initialize();
	renderer.SetBackgroundColor(0.203, 0.203, 0.203, 1.0);
	renderer.EnableTransparency();
	renderer.EnableSampleShading();

	particleShader = Shader::Create("particle", "assets/shaders/particle.vert", "assets/shaders/particle.frag");
	particleShader->noTexture();
	particleShader->noMaterial();
	particleShader->SetVec3("lightPos", glm::vec3(0, -200, 1000));

	binShader = Shader::Create("bins", "assets/shaders/bin.vert", "assets/shaders/bin.frag");
	binShader->noTexture();
	binShader->noMaterial();

	particleShader->Use();
	particleShader->SetInt("colorCycle", 3);
	binShader->Use();
	binShader->SetInt("colorCycle", 3);
	 
	renderer.AddShader(particleShader);
	renderer.AddShader(binShader);

	//qrenderer.SetShader(Shader("screen", "assets/shaders/screen.space.vert", "assets/shaders/screen.space.frag"));

	Model_Ptr mdl = Model::Create("bbox", Primitives::CreateQuadRectangle(settings.bb.x, settings.bb.y, true));
	mdl->EnableWireFrameMode();
	scene.Add(mdl);

	//scene.Add(TransformObject::Create("origin"));

}

void AppLayer::InitPhysics() {
	//Compute Shaders
	solver = StagedComputeShader::Create("solver", "assets/shaders/solver/solver.comp", 6);
	prefixSum = StagedComputeShader::Create("prefixSum", "assets/shaders/solver/prefix.sum.comp", 4);

	//Create particle system
	particleSystem = deprecated_ParticleSystem::Create("ParticleSystem", settings.pThread);
	Shared<Mesh> particle = Primitives::CreatePoint();
	particle->Rename("particle");
	particle->SetShader(particleShader);
	particleSystem->SetMesh(particle);
	particleSystem->SetDisplayMode(deprecated_ParticleSystemDisplayMode::POINT_SPRITE);

	Shared<Mesh> binInstance = Primitives::CreateQuadRectangle(settings.bWidth, settings.bWidth, true);
	binInstance->Rename("bin");
	binInstance->SetShader(binShader);
	binSystem = deprecated_ParticleSystem::Create("BinSystem", settings.bThread);
	binSystem->SetDisplayMode(deprecated_ParticleSystemDisplayMode::MESH);
	binSystem->SetMesh(binInstance);
	binSystem->EnableWireFrameMode();

	solver->SetWorkgroupLayout(settings.pWkgCount);
	prefixSum->SetWorkgroupLayout(settings.bWkgCount);

	// Allocate Buffer and double buffering
	positionBuffer = SSBO<glm::vec2>::Create("PositionBuffer", settings.pThread);
	cpyPositionBuffer = SSBO<glm::vec2>::Create("cpyPositionBuffer",settings.pThread);
	predictedPositionBuffer = SSBO<glm::vec2>::Create("PredictedPositionBuffer",settings.pThread);
	cpyPredictedPositionBuffer = SSBO<glm::vec2>::Create("cpyPredictedPositionBuffer",settings.pThread);
	velocityBuffer = SSBO<glm::vec2>::Create("VelocityBuffer",settings.pThread);
	cpyVelocityBuffer = SSBO<glm::vec2>::Create("cpyVelocityBuffer",settings.pThread);
	temperatureBuffer = SSBO<float>::Create("TemperatureBuffer", settings.pThread);
	cpyTemperatureBuffer = SSBO<float>::Create("cpyTemperatureBuffer",settings.pThread);
	metaBuffer = SSBO<glm::uvec4>::Create("MetaBuffer",settings.pThread);
	cpymetaBuffer = SSBO<glm::uvec4>::Create("cpyMetaBuffer",settings.pThread);


	Console::info() << "Bin struct size :" << sizeof(Bin) << Console::endl;
	binBuffer = SSBO<Bin>::Create("BinBuffer");
	binBuffer->Allocate(settings.bThread);
	binBuffer->Unbind();


	// Set binding points for position and its copy
	positionBuffer->SetBindingPoint(0);
	cpyPositionBuffer->SetBindingPoint(1);
	predictedPositionBuffer->SetBindingPoint(2);
	cpyPredictedPositionBuffer->SetBindingPoint(3);
	velocityBuffer->SetBindingPoint(4);
	cpyVelocityBuffer->SetBindingPoint(5);
	temperatureBuffer->SetBindingPoint(6);
	cpyTemperatureBuffer->SetBindingPoint(7);
	metaBuffer->SetBindingPoint(9);
	cpymetaBuffer->SetBindingPoint(10);
	binBuffer->SetBindingPoint(11);

	//Attach Buffers
	particleSystem->AddComputeShader(solver);
	particleSystem->AddStorageBuffer(positionBuffer);
	particleSystem->AddStorageBuffer(cpyPositionBuffer);
	particleSystem->AddStorageBuffer(predictedPositionBuffer);
	particleSystem->AddStorageBuffer(cpyPredictedPositionBuffer);
	particleSystem->AddStorageBuffer(velocityBuffer);
	particleSystem->AddStorageBuffer(cpyVelocityBuffer);
	particleSystem->AddStorageBuffer(temperatureBuffer);
	particleSystem->AddStorageBuffer(cpyTemperatureBuffer);
	particleSystem->AddStorageBuffer(metaBuffer);
	particleSystem->AddStorageBuffer(cpymetaBuffer);
	particleSystem->AddStorageBuffer(binBuffer);

	binSystem->AddComputeShader(prefixSum);
	binSystem->AddStorageBuffer(binBuffer);
	
	scene.Add(particleSystem);
	scene.Add(binSystem);
	//scene.Add(constraintSystem);
	binSystem->Hide();
}

void AppLayer::ResetSimulation() {
	elapsedTime = 0;
	positionBuffer->FreeHostMemory();
	predictedPositionBuffer->FreeHostMemory();
	velocityBuffer->FreeHostMemory();
	temperatureBuffer->FreeHostMemory();
	metaBuffer->FreeHostMemory();
	
	Console::info() << "Generating particles..." << Console::endl;

	float spacing = settings.particleRadius * 2.0;

	auto& cpu_position = positionBuffer->GetDeviceBuffer();
	auto& cpu_predictedPosition = predictedPositionBuffer->GetDeviceBuffer();
	auto& cpu_velocity = velocityBuffer->GetDeviceBuffer();
	auto& cpu_temperature = temperatureBuffer->GetDeviceBuffer();
	auto& cpu_meta = metaBuffer->GetDeviceBuffer();

	glm::vec2 cubeSize = glm::vec2(100, 250);
	glm::ivec2 icubeSize = glm::vec2(cubeSize.x / spacing, cubeSize.y / spacing);

	numParticles = 0;
	for (int yi = 0; yi <= cubeSize.y / spacing; yi++)
	for (int xi = 0; xi <= cubeSize.x / spacing; xi++){
		float x = ((xi + 1) * spacing) - (settings.bb.x/2.0);
		float y = ((yi + 1) * spacing) - (settings.bb.y/2.0);
		cpu_position.push_back(glm::vec2(x, y));
		cpu_predictedPosition.push_back(glm::vec2(x, y));
		cpu_velocity.push_back(glm::vec2(0));
		cpu_temperature.push_back(0.3*yi + 298.15);
		cpu_meta.push_back(glm::uvec4(GRANULAR, numParticles, numParticles, 0.0));
		numParticles++;
	}


	Console::info() << "Uploading buffer on device..." << Console::endl;
	settings.pThread = numParticles;
	particleSystem->SetInstancesCount(settings.pThread);
	
	ApplyBufferSettings();
	SyncUniforms();

	positionBuffer->Bind();
	positionBuffer->Upload();
	predictedPositionBuffer->Bind();
	positionBuffer->Upload();
	velocityBuffer->Bind();
	velocityBuffer->Upload();
	temperatureBuffer->Bind();
	temperatureBuffer->Upload();
	metaBuffer->Bind();
	metaBuffer->Upload();


}


void AppLayer::NeigborSearch() {
	prefixSum->Use();
	prefixSum->Execute(4);// clear bins

	solver->Use();
	solver->Execute(0); //Place particles in bins

	prefixSum->Use();
	prefixSum->Execute(0);// local prefix sum

	//Binary tree on rightmost element of blocks
	GLuint steps = settings.blockSize;
	UniformObject<GLuint> space("space");
	space.value = 1;
	
	for (GLuint step = 0; step < steps; step++) {
		// Calls the parallel operation

		space.Sync(*prefixSum);
		prefixSum->Execute(1);
		prefixSum->Execute(2);

		space.value *= 2;
	}
	prefixSum->Execute(3);

	solver->Use();
	solver->Execute(1); //Sort
}


void AppLayer::Simulate(Merlin::Timestep ts) {

	solver->Use();

	GPU_PROFILE(solver_substep_time,
	for (int i = 0; i < settings.solver_substep; i++) {
		solver->Execute(2);
		GPU_PROFILE(nns_time,
			NeigborSearch();
		)

		if (integrate) {
			GPU_PROFILE(jacobi_time,
				for (int j = 0; j < settings.solver_iteration; j++) {
					solver->Execute(3);
					solver->Execute(4);
				}
			)
			solver->Execute(5);

		}
	}
	)
	elapsedTime += settings.timestep.value();
	
}

void AppLayer::updateFPS(Timestep ts) {
	if (FPS_sample == 0) {
		FPS = ts;
	}
	else {
		FPS += ts;
	}
	FPS_sample++;
}



void AppLayer::OnImGuiRender() {
	//ImGui::ShowDemoWindow();
	ImGui::DockSpaceOverViewport((ImGuiViewport*)0, ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::Begin("Infos");


	ImGui::LabelText(std::to_string(numParticles).c_str(), "particles");
	ImGui::LabelText(std::to_string(settings.bThread).c_str(), "bins");
	ImGui::LabelText(std::to_string(elapsedTime).c_str(), "s");

	if (FPS_sample > 0) {
		ImGui::LabelText("FPS", std::to_string(1.0f / (FPS / FPS_sample)).c_str());
		if (FPS_sample > 50) FPS_sample = 0;
	}

	if (paused) {
		if (ImGui::ArrowButton("Run simulation", 1)) {
			paused = !paused;
		}
	}
	else {
		if (ImGui::SmallButton("Pause simulation")) {
			paused = !paused;
		}
	}


	static bool transparency = true;
	if (ImGui::Checkbox("Particle transparency", &transparency)) {
		if (transparency) particleSystem->SetDisplayMode(deprecated_ParticleSystemDisplayMode::POINT_SPRITE_SHADED);
		else particleSystem->SetDisplayMode(deprecated_ParticleSystemDisplayMode::POINT_SPRITE);
	}

	static bool showbed = true;
	if (ImGui::Checkbox("Show build plate", &showbed)) {
		auto bedRef = scene.GetChild("bed");
		if (bedRef) {
			if (showbed) scene.GetChild("bed")->Show();
			else scene.GetChild("bed")->Hide();
		}
	}

	ImGui::Checkbox("Integrate", &integrate);

	if (ImGui::SmallButton("Reset simulation")) {
		ResetSimulation();
	}

	static bool Pstate = true;
	if (ImGui::Checkbox("Show Particles", &Pstate)) {
		if (Pstate) particleSystem->Show();
		else particleSystem->Hide();
	}

	static bool Bstate = false;
	if (ImGui::Checkbox("Show Bins", &Bstate)) {
		if (Bstate) binSystem->Show();
		else binSystem->Hide();
	}

	static bool BBstate = false;
	if (ImGui::Checkbox("Show Boundaries", &BBstate)) {
		particleShader->Use();
		particleShader->SetInt("showBoundary", BBstate);
	}

	ImGui::DragInt("Solver substep", &settings.solver_substep, 1, 1, 200);
	ImGui::DragInt("Solver iteration", &settings.solver_iteration, 1, 1, 200);

	if (ImGui::DragFloat3("Camera position", &model_matrix_translation.x, -100.0f, 100.0f)) {
		camera->SetPosition(model_matrix_translation);
	}

	if (ImGui::InputFloat("Time step", &settings.timestep.value(), 0.0, 0.02f)) {
		solver->Use();
		solver->SetFloat("dt", settings.timestep.value());
	}



	if(ImGui::SliderFloat("Fluid particle mass", &settings.particleMass.value(), 0.1, 2.0)){
		solver->Use();
		settings.particleMass.Sync(*solver);
	}
	if (ImGui::SliderFloat("Rest density", &settings.restDensity.value(), 0.0, 2.0)){
		solver->Use();
		settings.restDensity.Sync(*solver);
		particleShader->Use();
		settings.restDensity.Sync(*particleShader);
	}
	if (ImGui::SliderFloat("Pressure multiplier", &settings.artificialPressureMultiplier.value(), 0.0, 10.0)){
		solver->Use();
		solver->SetFloat("artificialPressureMultiplier", settings.artificialPressureMultiplier.value() * 0.001);
	}
	if (ImGui::SliderFloat("Viscosity", &settings.artificialViscosityMultiplier.value(), 0.0, 10.0)){
		solver->Use();
		solver->SetFloat("artificialViscosityMultiplier", settings.artificialViscosityMultiplier.value() * 0.001);
	}

	static int colorMode = 3;
	static const char* options[] = { "Solid color", "Bin index", "Density", "Temperature", "Velocity", "Mass", "Neighbors" };
	if (ImGui::ListBox("Colored field", &colorMode, options, 7)) {
		particleShader->Use();
		particleShader->SetInt("colorCycle", colorMode);
		binShader->Use();
		binShader->SetInt("colorCycle", colorMode);
	}

	static int particleTest = 50;
	static int binTest = 1459;
	if (colorMode == 6) {
		if (ImGui::DragInt("Particle to test", &particleTest)) {
			particleShader->Use();
			particleShader->SetUInt("particleTest", particleTest);
			binShader->Use();
			binShader->SetUInt("particleTest", particleTest);
		}

		ImGui::LabelText("Bin selector", "");

		bool changed = false;

		ImGui::LabelText("Current Bin", std::to_string(binTest).c_str());

		if (ImGui::SmallButton("X+")) { binTest++; changed = true; }
		if (ImGui::SmallButton("X-")) { binTest--; changed = true; }
		if (ImGui::SmallButton("Y+")) { binTest += (settings.bb.x / settings.bWidth); changed = true; }
		if (ImGui::SmallButton("Y-")) { binTest -= (settings.bb.x / settings.bWidth); changed = true; }
		if (ImGui::SmallButton("Z+")) { binTest += int(settings.bb.y / settings.bWidth) * int(settings.bb.x / settings.bWidth); changed = true; }
		if (ImGui::SmallButton("Z-")) { binTest -= int(settings.bb.y / settings.bWidth) * int(settings.bb.x / settings.bWidth); changed = true; }

		if (changed) {
			binShader->Use();
			binShader->SetUInt("binTest", binTest);
		}
	}

	ImGui::End();

	ImGui::Begin("Performance");
	ImGui::Text("Nearest Neighbor Search %.1f ms", nns_time);
	ImGui::Text("Substep solver %.1f ms", solver_substep_time - nns_time);
	ImGui::Text("Jacobi iteration %.1f ms", jacobi_time);
	ImGui::Text("Total physics time %.1f ms", solver_total_time);
	ImGui::Text("Render time %.1f ms", render_time);
	ImGui::Text("Total frame time %.1f ms", total_time);
	ImGui::End();

	ImGui::Begin("Compute Shader");
	ImGui::LabelText("Solver", "");
	static int stepSolver = 0;
	ImGui::SliderInt("Solver Step", &stepSolver, 0.0, 5.0f);
	if (ImGui::Button("Execute Solver")) {
		solver->Use();
		solver->Execute(stepSolver);
	}

	ImGui::LabelText("PrefixSum", "");
	static int stepPrefix = 0;
	ImGui::LabelText("Solver", "");
	ImGui::SliderInt("PrefixSum Step", &stepPrefix, 0.0, 3.0f);
	if (ImGui::Button("Execute PrefixSum Step")) {
		prefixSum->Use();
		prefixSum->Execute(stepPrefix);
	}

	if (ImGui::Button("Step Simulation")) {
		GPU_PROFILE(solver_total_time,
			Simulate(0.016);
		)
	}

	if (ImGui::Button("Debug")) {
		positionBuffer->Bind();
		positionBuffer->Download();
		velocityBuffer->Bind();
		velocityBuffer->Download();
		predictedPositionBuffer->Bind();
		predictedPositionBuffer->Download();
		temperatureBuffer->Bind();
		temperatureBuffer->Download();
		metaBuffer->Bind();
		metaBuffer->Download();

		binBuffer->Bind();
		binBuffer->Download();

		std::vector<GLuint> sorted;
		
		for (int i = 0; i < numParticles; i++) {
			//sorted.push_back(particleBuffer->GetDeviceBuffer()[i].id);
			sorted.push_back(metaBuffer->GetDeviceBuffer()[i].w);
		}

		Console::info("Sorting") << "Sorted array has " << sorted.size() << " entry. " << numParticles - sorted.size() << " entry are missing" << Console::endl;

		std::unordered_map<GLuint, GLuint> idmap;
		for (int i = 0; i < sorted.size(); i++) {
			idmap[sorted[i]] = sorted[i];
		}

		std::vector<GLuint> missings;
		for (int i = 0; i < sorted.size(); i++) {
			if (idmap.find(i) == idmap.end()) {
				missings.push_back(i);
			};
		}


		Console::info("Sorting") << "Sorted array has " << sorted.size() << " entry. ID Map has " << idmap.size() << " entry" << Console::endl;

		throw("DEBUG");
		Console::info() << "DEBUG" << Console::endl;
	}
	ImGui::End();
}
