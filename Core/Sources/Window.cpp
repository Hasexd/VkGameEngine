#include "Window.h"

namespace Core
{
	Window::Window():
		m_Window(nullptr, nullptr),
		m_Width(0),
		m_Height(0)
	{ }

	Window::Window(const std::string& title, u32 width, u32 height): 
		m_Window(nullptr, glfwDestroyWindow),
		m_Width(width),
		m_Height(height)
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_Window.reset(glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(), nullptr, nullptr));
		ASSERT(m_Window);
	}
}

