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
	Application::Application(const std::string& title, u32 width, u32 height)
	{
		glfwSetErrorCallback(GLFWErrorCallback);

		bool ok = glfwInit();

		if (!ok)
		{
			LOG_CRITICAL("Failed to initialize GLFW.");
			return;
		}

		LOG_INFO("GLFW initialized successfully.");

		m_Window = Window(title, width, height);
	}

	void Application::Run()
	{
		while (!glfwWindowShouldClose(m_Window.GetNativeWindow()))
		{
			//glfwSwapBuffers(m_Window.GetNativeWindow());
			glfwPollEvents();
		}

		glfwTerminate();
	}
}