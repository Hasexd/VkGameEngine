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
		if (s_Instance)
		{
			LOG_CRITICAL("Application already exists!");
			return;
		}

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
		m_LayerStack.clear();

		m_Renderer->Cleanup();
		m_Renderer.reset();

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

			m_FPS = 1.0f / m_DeltaTime;

			glfwPollEvents();

			if (m_Window.ShouldClose())
			{
				m_Running = false;
				break;
			}

			int width = 0, height = 0;
			glfwGetFramebufferSize(m_Window.GetHandle(), &width, &height);

			if (width == 0 || height == 0)
			{
				continue;
			}

			for(const auto& layer : m_LayerStack)
				layer->OnUpdate(m_DeltaTime);

			m_PreFrameRenderFunction();
			m_Renderer->BeginFrame();
			m_Renderer->BeginRenderToTexture();

			for(const auto& layer : m_LayerStack)
				layer->OnRender();

			m_Renderer->EndRenderToTexture();

			m_Renderer->BeginRenderToSwapchain();

			for(const auto& layer : m_LayerStack)
				layer->OnSwapchainRender();

			m_Renderer->EndRenderToSwapchain();

			m_Renderer->EndFrame();

			while (!m_PostFrameEventQueue.empty())
			{
				auto& event = m_PostFrameEventQueue.front();
				RaiseEvent(*event);
				m_PostFrameEventQueue.pop();
			}
		}
		vkDeviceWaitIdle(m_Renderer->GetDevice());
	}

	void Application::RaiseEvent(Event& event)
	{
		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<TransitionLayerEvent>([this](TransitionLayerEvent& e) 
		{
			for (auto& layer : m_LayerStack)
			{
				if (layer.get() == e.GetFromLayer())
				{
					layer = e.Transition();
					return true;
				}
			}
			return false;
			});

		for (auto& layer : std::views::reverse(m_LayerStack))
		{
			layer->OnEvent(event);
			if (event.Handled)
				break;
		}
	}

	void Application::QueuePostFrameEvent(std::unique_ptr<Event> event)
	{
		m_PostFrameEventQueue.push(std::move(event));
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

	void Application::SetCursorState(i32 state)
	{
		glfwSetInputMode(s_Instance->m_Window.GetHandle(), GLFW_CURSOR, state);
	}
	
	i32 Application::GetCursorState()
	{
		return glfwGetInputMode(s_Instance->m_Window.GetHandle(), GLFW_CURSOR);
	}

	VkCommandBuffer Application::GetCurrentCommandBuffer()
	{
		return s_Instance->m_Renderer->GetCurrentCommandBuffer();
	}

	VkPipelineLayout Application::GetGraphicsPipelineLayout()
	{
		return s_Instance->m_Renderer->GetGraphicsPipelineLayout();
	}

	void Application::SetBackgroundColor(const VkClearColorValue& color)
	{
		s_Instance->m_Renderer->SetBackgroundColor(color);
	}

	MeshBuffers Application::CreateMeshBuffers(const std::vector<objl::Vertex>& vertices, const std::vector<u32>& indices)
	{
		return s_Instance->m_Renderer->CreateMeshBuffers(vertices, indices);
	}
}