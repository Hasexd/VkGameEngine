#pragma once

#include <memory>
#include <string>
#include <vector>
#include <ranges>
#include <queue>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "Types.h"
#include "Log.h"
#include "Renderer.h"
#include "UUID.h"
#include "Object.h"
#include "ECS.h"
#include "Layer.h"
#include "Window.h"

namespace Core
{
	class Application
	{
	public:
		Application(const std::string& title, u32 width, u32 height);
		~Application();

		static Application& Get();
		static f32 GetDeltaTime();
		static Window& GetWindow();

		static MeshBuffers CreateMeshBuffers(const std::vector<Vertex>& vertices, const std::vector<u32>& indices);
		static Mesh CreateMeshFromOBJ(const std::string& objName);

		i32 GetCursorState();
		void SetCursorState(i32 state);

		VkCommandBuffer GetCurrentCommandBuffer();
		VkPipelineLayout GetGraphicsPipelineLayout();
		void SetBackgroundColor(const VkClearColorValue& color);

		void SetPreFrameRenderFunction(const std::function<void()>& func) { m_PreFrameRenderFunction = func; };

		void SetWireframeMode(const bool enabled) { m_Renderer->SetWireframeMode(enabled); };

		void Run();
		void RaiseEvent(Event& event);

		void QueuePostFrameEvent(std::unique_ptr<Event> event);

		template<std::derived_from<Layer> T>
		void PushLayer();

		template<std::derived_from<Layer> T>
		T* GetLayer();

		[[nodiscard]] VkInstance GetVulkanInstance() const { return m_Renderer->GetVulkanInstance(); }
		[[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return m_Renderer->GetPhysicalDevice(); }
		[[nodiscard]] VkDevice GetVulkanDevice() const { return m_Renderer->GetVulkanDevice(); }
		[[nodiscard]] vkb::Swapchain GetSwapchain() const { return m_Renderer->GetSwapchain(); }
		[[nodiscard]] u32 GetQueueFamily() const { return m_Renderer->GetQueueFamily(); };
		[[nodiscard]] VkQueue GetGraphicsQueue() const { return m_Renderer->GetGraphicsQueue(); }
		[[nodiscard]] u32 GetSwapchainImageCount() const { return m_Renderer->GetSwapchainImageCount(); }
		[[nodiscard]] VkRenderPass GetRenderPass() const { return m_Renderer->GetRenderPass(); }
		[[nodiscard]] VkRenderPass GetRenderTextureRenderPass() const { return m_Renderer->GetRenderTextureRenderPass(); }
		[[nodiscard]] VkSampler GetRenderTextureSampler() const { return m_Renderer->GetRenderTextureSampler(); }
		[[nodiscard]] VkImageView GetRenderTextureImageView() const { return m_Renderer->GetRenderTextureImageView(); }
		[[nodiscard]] VmaAllocator GetVmaAllocator() const { return m_Renderer->GetVmaAllocator(); }

		[[nodiscard]] Shader CreateShader(const VkRenderPass& renderPass, const std::vector<DescriptorBinding>& bindings,
			const std::vector<VkPushConstantRange>& pushConstantRanges,
			VkVertexInputBindingDescription* vtxInputBindingDesc,
			const std::vector<VkVertexInputAttributeDescription>& vtxInputAttrDesc,
			VkViewport* viewport,
			VkRect2D* scissor,
			VkPipelineDepthStencilStateCreateInfo* depthStencilInfo,
			const std::vector<VkDynamicState>& dynamicStates,
			VkCullModeFlagBits cullMode,
			VkPrimitiveTopology topology,
			const std::filesystem::path& vert, const std::filesystem::path& frag) const { return m_Renderer->CreateShader(renderPass, bindings, pushConstantRanges, vtxInputBindingDesc, vtxInputAttrDesc,
				viewport, scissor, depthStencilInfo, dynamicStates, cullMode, topology, vert, frag); }

		[[nodiscard]] Buffer& GetVPBuffer() { return m_Renderer->GetVPBuffer(); }

		void UpdateDescriptorSets(const Shader& shader) { m_Renderer->UpdateDescriptorSets(shader); }

		Buffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) { return m_Renderer->CreateBuffer(size, usage, memoryUsage); };
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) { m_Renderer->CopyBuffer(srcBuffer, dstBuffer, size); };

	private:
		static inline Application* s_Instance = nullptr;

		f32 m_DeltaTime;
		bool m_Running = true;

		Window m_Window;
		std::unique_ptr<Renderer> m_Renderer;
		std::vector<std::unique_ptr<Layer>> m_LayerStack;
		std::queue<std::unique_ptr<Event>> m_PostFrameEventQueue;

		std::function<void()> m_PreFrameRenderFunction;
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