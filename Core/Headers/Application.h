#pragma once

#include <memory>
#include <string>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "Types.h"
#include "Log.h"
#include "Renderer.h"
#include "UUID.h"

namespace Core
{
	class Application
	{
	public:
		Application(const std::string& title, u32 width, u32 height);
		~Application();

		void Run();

	private:
		std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> m_Window;
		std::unique_ptr<Renderer> m_Renderer;
	};
}