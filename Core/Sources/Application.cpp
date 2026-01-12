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
		glfwSetWindowUserPointer(m_Window.get(), this);

		ASSERT(m_Window);

		m_Renderer->Init(m_Window.get());

		m_Camera.AspectRatio = static_cast<f32>(width) / static_cast<f32>(height);

		objl::Loader loader;
		std::filesystem::path objPath = std::filesystem::path(PATH_TO_OBJS) / "Cube.obj";

		loader.LoadFile(objPath.string());


		auto obj = AddObject();
		obj->GetComponent<Transform>()->Position = {0.0f, 0.0f, -3.0f};
		CreateMesh(obj, loader.LoadedVertices, loader.LoadedIndices);
	}

	Application::~Application()
	{
		m_Window.reset();
		glfwTerminate();
	}

	void Application::Run()
	{
		glfwSetCursorPosCallback(m_Window.get(), [](GLFWwindow* window, double xpos, double ypos)
		{
			static double lastX = xpos;
			static double lastY = ypos;
			double xOffset = xpos - lastX;
			double yOffset = ypos - lastY;
			lastX = xpos;
			lastY = ypos;
			Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
			app->m_Camera.Rotate(xOffset, yOffset, app->m_DeltaTime);
		});

		glfwSetInputMode(m_Window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		double lastFrame = glfwGetTime();
		while (!glfwWindowShouldClose(m_Window.get()))
		{
			double currentFrame = glfwGetTime();
			m_DeltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;

			glfwPollEvents();

			if(glfwGetKey(m_Window.get(), GLFW_KEY_W) == GLFW_PRESS)
				m_Camera.Move(m_Camera.Front, m_DeltaTime);
			if (glfwGetKey(m_Window.get(), GLFW_KEY_S) == GLFW_PRESS)
				m_Camera.Move(-m_Camera.Front, m_DeltaTime);
			if (glfwGetKey(m_Window.get(), GLFW_KEY_A) == GLFW_PRESS)
				m_Camera.Move(-m_Camera.Right, m_DeltaTime);
			if (glfwGetKey(m_Window.get(), GLFW_KEY_D) == GLFW_PRESS)
				m_Camera.Move(m_Camera.Right, m_DeltaTime);
			if (glfwGetKey(m_Window.get(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
				glfwSetInputMode(m_Window.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

			m_Renderer->DrawFrame(m_Objects, m_Camera);
		}
		vkDeviceWaitIdle(m_Renderer->GetDevice());
	}

	std::shared_ptr<Object> Application::AddObject()
	{
		return m_Objects.emplace_back(std::make_shared<Object>(m_ECS));
	}

	void Application::CreateMesh(const std::shared_ptr<Object>& obj, const std::vector<Vertex>& vertices, const std::vector<u32>& indices)
	{
		obj->AddComponent<Mesh>(m_Renderer->CreateMeshBuffers(vertices, indices));
	}
}