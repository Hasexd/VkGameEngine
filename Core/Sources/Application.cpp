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
		while (!glfwWindowShouldClose(m_Window.get()))
		{
			glfwPollEvents();
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