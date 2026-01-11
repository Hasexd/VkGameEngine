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

		m_Camera.AspectRatio = static_cast<f32>(width) / static_cast<f32>(height);

		m_Objects.push_back(std::make_shared<Object>());

		std::vector<Vertex> vertices =
		{
			{{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
			{{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
			{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
			{{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
			{{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
			{{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
			{{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
			{{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}
		};
		std::vector<u32> indices =
		{
			 0, 1, 2, 2, 3, 0,
			 5, 4, 7, 7, 6, 5,
			 4, 0, 3, 3, 7, 4,
			 1, 5, 6, 6, 2, 1,
			 3, 2, 6, 6, 7, 3,
			 4, 5, 1, 1, 0, 4
		};

		Mesh& mesh = m_Objects[0]->GetMesh();
		mesh = m_Renderer->CreateMeshBuffers(vertices, indices);
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
			m_Renderer->DrawFrame(m_Objects, m_Camera);
		}
		vkDeviceWaitIdle(m_Renderer->GetDevice());
	}
}