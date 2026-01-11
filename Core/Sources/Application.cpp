#include "Application.h"

namespace
{
	void GLFWErrorCallback(int error, const char* desc)
	{
		LOG_ERROR("Error: {}", desc);
	}
}

namespace Core
{
	Application::Application(const std::string& title, u32 width, u32 height) :
		m_Window(nullptr, glfwDestroyWindow),
		m_Renderer(std::make_unique<Renderer>())
	{
		glfwSetErrorCallback(GLFWErrorCallback);

		bool ok = glfwInit();

		if (!ok)
		{
			LOG_CRITICAL("Failed to initialize GLFW.");
			return;
		}

		LOG_INFO("GLFW initialized successfully.");

		if (!glfwVulkanSupported())
		{
			LOG_CRITICAL("Vulkan is not supported on this system.");
			return;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_Window.reset(glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(), nullptr, nullptr));

		ASSERT(m_Window);

		m_Renderer->Init(m_Window.get());
	}

	Application::~Application()
	{
		m_Window.reset();
		glfwTerminate();
	}

	void Application::Run()
	{
		while (!glfwWindowShouldClose(m_Window.get()))
		{
			glfwPollEvents();
			m_Renderer->DrawFrame();
		}
		vkDeviceWaitIdle(m_Renderer->GetDevice());
	}
}