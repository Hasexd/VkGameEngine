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
	Application::Application(const std::string& title, u32 width, u32 height):
		m_Renderer(std::make_unique<Renderer>())
	{
		s_Instance = this;
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

		m_Window.Create(title, width, height);
		m_Window.SetEventCallback([this](Event& event) { RaiseEvent(event); });
		m_Renderer->Init(m_Window.GetHandle());
	}

	Application::~Application()
	{
		m_Window.Destroy();
		glfwTerminate();
		s_Instance = nullptr;
	}

	void Application::Run()
	{
		f32 lastFrame = glfwGetTime();
		while (m_Running)
		{
			f32 currentFrame = glfwGetTime();
			m_DeltaTime = glm::clamp(currentFrame - lastFrame, 0.001f, 0.1f);
			lastFrame = currentFrame;

			glfwPollEvents();

			if (m_Window.ShouldClose())
			{
				m_Running = false;
				break;
			}

			for(const auto& layer : m_LayerStack)
				layer->OnUpdate(m_DeltaTime);

			m_Renderer->BeginFrame();
			m_Renderer->BeginRenderToTexture();

			for(const auto& layer : m_LayerStack)
				layer->OnRender();

			m_Renderer->EndRenderToTexture();

			m_Renderer->BeginRenderToSwapchain();
			m_Renderer->EndRenderToSwapchain();

			m_Renderer->EndFrame();
		}
		vkDeviceWaitIdle(m_Renderer->GetDevice());
	}

	void Application::SetInputMode(i32 mode)
	{
		glfwSetInputMode(s_Instance->m_Window.GetHandle(), GLFW_CURSOR, mode);
	}

	void Application::RaiseEvent(Event& event)
	{
		for (auto& layer : std::views::reverse(m_LayerStack))
		{
			layer->OnEvent(event);
			if (event.Handled)
				break;
		}
	}

	Application& Application::Get()
	{
		return *s_Instance;
	}

	f32 Application::GetDeltaTime()
	{
		return s_Instance ? s_Instance->m_DeltaTime : 0.0;
	}

	Window& Application::GetWindow()
	{
		return s_Instance->m_Window;
	}

	VkCommandBuffer Application::GetCurrentCommandBuffer()
	{
		return s_Instance->m_Renderer->GetCurrentCommandBuffer();
	}

	VkPipelineLayout Application::GetGraphicsPipelineLayout()
	{
		return s_Instance->m_Renderer->GetGraphicsPipelineLayout();
	}

	std::vector<std::unique_ptr<Layer>>& Application::GetLayerStack()
	{
		return s_Instance->m_LayerStack;
	}

	MeshBuffers Application::CreateMeshBuffers(const std::vector<Vertex>& vertices, const std::vector<u32>& indices)
	{
		return s_Instance->m_Renderer->CreateMeshBuffers(vertices, indices);
	}

	Mesh Application::CreateMeshFromOBJ(const std::string& objName)
	{
		objl::Loader loader;
		std::filesystem::path objPath = std::filesystem::path(PATH_TO_OBJS) / objName;
		loader.LoadFile(objPath.string());

		MeshBuffers buffers = CreateMeshBuffers(loader.LoadedVertices, loader.LoadedIndices);
		return Mesh(buffers);
	}
}