#include "glpch.h"
#include "Application.h"

#include "Log.h"
#include "Input.h"

#include <glfw/glfw3.h>
#include <glad/glad.h>

namespace Merlin {

#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	Application::Application(const std::string& name, uint32_t width, uint32_t height)
	{
		if (!s_Instance)
		{
			// Initialize core
			//LOG_INFO("Application") << "Initializing core" << Console::endl;
		}
 
		GLCORE_ASSERT(!s_Instance, "Application", "Application already exists!");
		s_Instance = this;

		m_Window = std::unique_ptr<Window>(Window::Create({ name, width, height }));
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));

		int deviceMem;
		int availableMem;
		glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &deviceMem);
		glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &availableMem);
		
		Console::print() 
			<< "| -------------- | " << "------------------------------------------------------------"	<< "|" << Console::endl
			<< "| Merlin Engine  | " << Console::alignLeft(60, "v1.0.0")								<< "|" << Console::endl
			<< "| -------------- | " << "------------------------------------------------------------"	<< "|" << Console::endl
			<< "| Device vendor  | " << Console::alignLeft(60, (const char*)glGetString(GL_VENDOR))		<< "|" << Console::endl
			<< "| Device name    | " << Console::alignLeft(60, (const char*)glGetString(GL_RENDERER))	<< "|" << Console::endl
			<< "| OpenGL version | " << Console::alignLeft(60, (const char*)glGetString(GL_VERSION))	<< "|" << Console::endl
			<< "| GLSL version   | " << Console::alignLeft(60, (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION)) << "|" << Console::endl
			<< "| Device RAM     | " << Console::alignLeft(60, std::to_string(availableMem / 1000000.0f) + " / " + std::to_string(deviceMem / 1000000) + " Go") << "|" << Console::endl
			<< "| -------------- | " << "------------------------------------------------------------"	<< "|" << Console::endl;

		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
		{
			(*--it)->OnEvent(e);
			if (e.Handled)
				break;
		}
	}

	void Application::Run()
	{
		while (m_Running)
		{
			float time = (float)glfwGetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			for (Layer* layer : m_LayerStack)
				layer->OnUpdate(timestep);

			m_ImGuiLayer->Begin();
			for (Layer* layer : m_LayerStack)
				layer->OnImGuiRender();
			m_ImGuiLayer->End();

			m_Window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

}