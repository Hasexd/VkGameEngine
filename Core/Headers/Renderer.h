#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include <array>
#include <functional>
#include <cstddef>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include "Log.h"
#include "Types.h"
#include "VkTypes.h"
#include "Mesh.h"
#include "Transform.h"
#include "Object.h"
#include "Camera.h"

constexpr usize MAX_FRAMES_IN_FLIGHT = 2;

namespace Core
{
	class Renderer
	{
	public:
		Renderer() = default;

		void Init(GLFWwindow* window);
		void Cleanup();

		void BeginFrame();
		void EndFrame();

		void BeginRenderToTexture();
		void EndRenderToTexture();

		void BeginRenderToSwapchain();
		void EndRenderToSwapchain();

		void SetBackgroundColor(const VkClearColorValue& color) { m_ClearColor = color; };
		void SetWireframeMode(const bool enabled) { m_WireframeMode = enabled; }

		MeshBuffers CreateMeshBuffers(const std::vector<Vertex>& vertices, const std::vector<u32>& indices);

		[[nodiscard]] VkCommandBuffer GetCurrentCommandBuffer() const { return m_CurrentCommandBuffer; }
		[[nodiscard]] VkPipelineLayout GetGraphicsPipelineLayout() const { return m_GraphicsShader.PipelineLayout; }

		[[nodiscard]] vkb::Device GetDevice() const noexcept { return m_CoreData.Device; }
		[[nodiscard]] VkInstance GetVulkanInstance() const { return m_CoreData.Instance; }
		[[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return m_CoreData.PhysicalDevice; }
		[[nodiscard]] VkDevice GetVulkanDevice() const { return m_CoreData.Device; }
		[[nodiscard]] vkb::Swapchain GetSwapchain() const { return m_CoreData.Swapchain; }
		[[nodiscard]] u32 GetQueueFamily() const { return m_RenderData.QueueFamily; };
		[[nodiscard]] VkQueue GetGraphicsQueue() const { return m_RenderData.GraphicsQueue; }
		[[nodiscard]] u32 GetSwapchainImageCount() const { return static_cast<u32>(m_RenderData.SwapchainImages.size()); }
		[[nodiscard]] VkRenderPass GetRenderPass() const { return m_RenderData.RenderPass; }
		[[nodiscard]] VkRenderPass GetRenderTextureRenderPass() const { return m_RenderTextureRenderPass; }
		[[nodiscard]] VkSampler GetRenderTextureSampler() const { return m_RenderTextureSampler; }
		[[nodiscard]] VkImageView GetRenderTextureImageView() const { return m_RenderTexture.View; }
		[[nodiscard]] VmaAllocator GetVmaAllocator() const { return m_Allocator; }

		[[nodiscard]] Buffer& GetVPBuffer() { return m_VPBuffer; }


		Shader CreateShader(const VkRenderPass& renderPass, const std::vector<DescriptorBinding>& bindings,
			const std::vector<VkPushConstantRange>& pushConstantRanges,
			VkVertexInputBindingDescription* vtxInputBindingDesc,
			const std::vector<VkVertexInputAttributeDescription>& vtxInputAttrDesc,
			VkViewport* viewport,
			VkRect2D* scissor,
			VkPipelineDepthStencilStateCreateInfo* depthStencilInfo,
			const std::vector<VkDynamicState>& dynamicStates,
			VkCullModeFlagBits cullMode,
			VkPrimitiveTopology topology,
			const std::filesystem::path& vert,
			const std::filesystem::path& frag,
			const std::filesystem::path& geom = "");

		void UpdateDescriptorSets(const Shader& shader);

		Buffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	private:
		void InitCoreData();
		void CreateBuffers();
		void CreateSwapchain();
		void GetQueues();
		void CreateDepthResources();
		void CreateRP();
		void CreateGP();
		void CreateBlitPipeline();
		void CreateRenderTextures();
		void CreateFramebuffers();
		void CreateCommandPool();
		void CreateCommandBuffers();
		void CreateImmediateCommandResources();
		void CreateSyncObjects();
		void RecreateSwapchain();

		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

		Image CreateImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageAspectFlags aspects,
			VkImageUsageFlags usage, VmaMemoryUsage memoryUsage);

		void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	private:
		GLFWwindow* m_Window;

		CoreData m_CoreData;
		RenderData m_RenderData;

		VkCommandBuffer m_CurrentCommandBuffer;
		u32 m_CurrentImageIndex;

		bool m_FrameInProgress = false;
		bool m_WireframeMode = false;

		VkCommandPool m_ImmediateCommandPool;
		VkCommandBuffer m_ImmediateCommandBuffer;

		Image m_DepthImage;

		Shader m_GraphicsShader;
		Shader m_WireframeShader;
		Shader m_BlitShader;

		Image m_RenderTexture;
		VkSampler m_RenderTextureSampler;
		VkRenderPass m_RenderTextureRenderPass;
		VkFramebuffer m_RenderTextureFramebuffer;

		Buffer m_VPBuffer;

		VmaAllocator m_Allocator;

		VkClearColorValue m_ClearColor = {0.0f, 0.0f, 0.0f, 1.0f};

		std::filesystem::path m_ShaderDirectory = std::filesystem::path(PATH_TO_SHADERS);

	};
}

