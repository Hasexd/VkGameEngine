#pragma once

#include <memory>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "Types.h"
#include "Log.h"
#include "Renderer.h"
#include "UUID.h"
#include "Object.h"
#include "ECS.h"
#include "Layer.h"

namespace Core
{
	class Application
	{
	public:
		Application(const std::string& title, u32 width, u32 height);
		~Application();

		static Application& Get();
		static f32 GetDeltaTime();

		static GLFWwindow* GetWindow();
		static glm::vec2 GetFramebufferSize();

		static VkCommandBuffer GetCurrentCommandBuffer();
		static VkPipelineLayout GetGraphicsPipelineLayout();

		static std::vector<std::unique_ptr<Layer>>& GetLayerStack();

		static MeshBuffers CreateMeshBuffers(const std::vector<Vertex>& vertices, const std::vector<u32>& indices);
		static Mesh CreateMeshFromOBJ(const std::string& objName);

		void Run();

		template<std::derived_from<Layer> T>
		void PushLayer();

		template<std::derived_from<Layer> T>
		T* GetLayer();

	private:
		static inline Application* s_Instance = nullptr;
		f32 m_DeltaTime;
		bool m_Running = true;

		std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> m_Window;
		std::unique_ptr<Renderer> m_Renderer;
		std::vector<std::unique_ptr<Layer>> m_LayerStack;
	};

	template<std::derived_from<Layer> T>
	void Application::PushLayer()
	{
		m_LayerStack.emplace_back(std::make_unique<T>());
	}

	template<std::derived_from<Layer> T>
	T* Application::GetLayer()
	{
		for (const auto& layer : m_LayerStack)
		{
			if (auto casted = dynamic_cast<T*>(layer.get()))
				return casted;
		}
		return nullptr;
	}
}