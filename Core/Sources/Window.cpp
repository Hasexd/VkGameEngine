#include "Window.h"

namespace Core
{
	Window::Window():
		m_Handle(nullptr, glfwDestroyWindow) { }

	void Window::Create(const std::string& title, u32 width, u32 height)
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_Title = title;
		m_Width = width;
		m_Height = height;

		m_Handle.reset(glfwCreateWindow(m_Width, m_Height,
			m_Title.c_str(), nullptr, nullptr));

		ASSERT(m_Handle);

		glfwSetWindowUserPointer(GetHandle(), this);

		glfwSetWindowCloseCallback(GetHandle(), [](GLFWwindow* handle)
			{
				Window& window = *((Window*)glfwGetWindowUserPointer(handle));

				WindowClosedEvent event;
				window.RaiseEvent(event);
			});

		glfwSetWindowSizeCallback(GetHandle(), [](GLFWwindow* handle, int width, int height)
			{
				Window& window = *((Window*)glfwGetWindowUserPointer(handle));

				WindowResizeEvent event((uint32_t)width, (uint32_t)height);
				window.RaiseEvent(event);
			});

		glfwSetKeyCallback(GetHandle(), [](GLFWwindow* handle, int key, int scancode, int action, int mods)
			{
				Window& window = *((Window*)glfwGetWindowUserPointer(handle));

				switch (action)
				{
				case GLFW_PRESS:
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, action == GLFW_REPEAT);
					window.RaiseEvent(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					window.RaiseEvent(event);
					break;
				}
				}
			});

		glfwSetMouseButtonCallback(GetHandle(), [](GLFWwindow* handle, int button, int action, int mods)
			{
				Window& window = *((Window*)glfwGetWindowUserPointer(handle));

				switch (action)
				{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					window.RaiseEvent(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					window.RaiseEvent(event);
					break;
				}
				}
			});

		glfwSetScrollCallback(GetHandle(), [](GLFWwindow* handle, double xOffset, double yOffset)
			{
				Window& window = *((Window*)glfwGetWindowUserPointer(handle));

				MouseScrolledEvent event(xOffset, yOffset);
				window.RaiseEvent(event);
			});

		glfwSetCursorPosCallback(GetHandle(), [](GLFWwindow* handle, double x, double y)
			{
				Window& window = *((Window*)glfwGetWindowUserPointer(handle));

				MouseMovedEvent event(x, y);
				window.RaiseEvent(event);
			});
	}

	void Window::RaiseEvent(Event& event)
	{
		if (m_EventCallback)
			m_EventCallback(event);
	}

	Window::~Window()
	{
		Destroy();
	}

	void Window::Destroy()
	{
		if (m_Handle)
		{
			m_Handle.reset();
			m_Handle = nullptr;
		}
	}

	glm::vec2 Window::GetFramebufferSize() const
	{
		int width, height;
		glfwGetFramebufferSize(GetHandle(), &width, &height);
		return { width, height };
	}

	glm::vec2 Window::GetMousePos() const
	{
		double x, y;
		glfwGetCursorPos(GetHandle(), &x, &y);
		return { static_cast<float>(x), static_cast<float>(y) };
	}

	bool Window::ShouldClose() const
	{
		return glfwWindowShouldClose(GetHandle()) != 0;
	}
}