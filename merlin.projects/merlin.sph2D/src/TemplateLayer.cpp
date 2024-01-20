#include "TemplateLayer.h"

using namespace Merlin;
using namespace Merlin::Utils;
using namespace Merlin::Memory;
using namespace Merlin::Graphics;

#include <iostream>
#include <iomanip>

TemplateLayer::TemplateLayer(){
	Window* w = &Application::Get().GetWindow();
	int height = w->GetHeight();
	int width = w->GetWidth();
	camera = CreateShared<Camera>(width, height, Projection::Orthographic);
	camera->setNearPlane(0.0f);
	camera->setFarPlane(1.0f);
	camera->setFOV(45.0f); //Use 90.0f as we are using cubemaps
	camera->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
}

TemplateLayer::~TemplateLayer(){}

void TemplateLayer::OnAttach(){
	EnableGLDebugging();
	Console::SetLevel(ConsoleLevel::_INFO);

	renderer.Initialize();
	renderer.DisableDepthTest();
	renderer.EnableSampleShading();
	renderer.SetBackgroundColor(0.203, 0.203, 0.203, 1.0);

	//modelShader = Shader::Create("default", "assets/common/shaders/default.model.vert", "assets/common/shaders/default.model.frag");
	modelShader = Shader::Create("model", "assets/common/shaders/default.model.vert", "assets/common/shaders/default.model.frag");
	modelShader->noTexture();
	renderer.AddShader(modelShader);

	Shared<Model> model = Model::Create("sphere1", Primitives::CreateSphere(0.5, 40, 40));
	model->SetMaterial("gold");
	model->SetShader("model");
	scene.Add(model);

	light = Model::Create("light", Primitives::CreateSphere(0.05));
	Shared<Material> lightMat = CreateShared<Material>("lightMat");
	lightMat->SetAmbient(glm::vec3(1));
	lightMat->SetDiffuse(glm::vec3(1));
	lightMat->SetSpecular(glm::vec3(1));
	lightMat->SetShininess(0.1);
	light->SetMaterial(lightMat);

	light->Translate(glm::vec3(1, 1, 0));

	scene.Add(light);

	modelShader->Use();
	modelShader->SetVec3("lightPos", light->position());
	modelShader->SetVec4("lightColor", glm::vec4(1));

	scene.SetCamera(camera);
}

void TemplateLayer::OnDetach(){}

void TemplateLayer::OnEvent(Event& event){
	camera->OnEvent(event);
}

float t = 0.0;

void TemplateLayer::OnUpdate(Timestep ts){

	t += ts;
	float x = light->position().x;
	float y = light->position().y;
	light->Translate(glm::vec3(cos(t) - x, sin(t) - y, 0.0));
	modelShader->Use();
	modelShader->SetVec3("lightPos", light->position());

	renderer.Clear();
	renderer.RenderScene(scene, *camera);
}

void TemplateLayer::OnImGuiRender()
{
	ImGui::Begin("Camera");

	model_matrix_translation = camera->GetPosition();
	if (ImGui::DragFloat3("Camera position", &model_matrix_translation.x, -100.0f, 100.0f)) {
		camera->SetPosition(model_matrix_translation);
	}
	ImGui::End();


}