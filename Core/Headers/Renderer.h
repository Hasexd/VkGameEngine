#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include <array>
#include <functional>
#include <cstddef>
#include <memory>
#include <ranges>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include "Log.h"
#include "Types.h"
#include "VkTypes.h"
#include "Transform.h"
#include "Object.h"
#include "Camera.h"


constexpr usize MAX_FRAMES_IN_FLIGHT = 2;

namespace Core
{
	enum class TimestampType
	{
		RenderThread
	};

	struct Timestamp
	{
		u64 Start, End;
		VkQueryPool QueryPool;
	};

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

		MeshBuffers CreateMeshBuffers(const std::vector<objl::Vertex>& vertices, const std::vector<u32>& indices);

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
		[[nodiscard]] VkSampler GetRenderTextureSampler() const { return m_RenderTextureSampler; }
		[[nodiscard]] VkImageView GetRenderTextureImageView() const { return m_RenderTexture.View; }
		[[nodiscard]] VmaAllocator GetVmaAllocator() const { return m_Allocator; }

		[[nodiscard]] Buffer& GetVPBuffer() { return m_VPBuffer; }
		[[nodiscard]] Buffer& GetMaterialsBuffer() { return m_MaterialsBuffer; }
		[[nodiscard]] VkSampleCountFlagBits GetMSAASamples() const { return m_MSAASamples; }
		[[nodiscard]] VkPhysicalDeviceLimits GetPhysicalDeviceLimits() const { return m_PhysDeviceLimits; }

		[[nodiscard]] Shader& GetGraphicsShader() { return m_GraphicsShader; }

		[[nodiscard]] const f32 GetGPUTime(const TimestampType& type) const
		{ return static_cast<f32>(m_Timestamps.at(type).End - m_Timestamps.at(type).Start) * m_PhysDeviceLimits.timestampPeriod / 1000000.0f; }

		[[nodiscard]] VkPipelineRenderingCreateInfoKHR GetGraphicsRenderingInfo() const;
		[[nodiscard]] VkPipelineRenderingCreateInfoKHR GetSwapchainRenderingInfo() const;

		Shader CreateShader(VkPipelineRenderingCreateInfoKHR* renderingInfo, const std::vector<DescriptorBinding>& bindings,
			const std::vector<VkPushConstantRange>& pushConstantRanges,
			VkVertexInputBindingDescription* vtxInputBindingDesc,
			const std::vector<VkVertexInputAttributeDescription>& vtxInputAttrDesc,
			VkViewport* viewport,
			VkRect2D* scissor,
			VkPipelineDepthStencilStateCreateInfo* depthStencilInfo,
			const std::vector<VkDynamicState>& dynamicStates,
			VkPipelineMultisampleStateCreateInfo* multisampleInfo,
			VkCullModeFlagBits cullMode,
			VkPolygonMode polygonMode,
			VkPrimitiveTopology topology,
			const std::filesystem::path& vert,
			const std::filesystem::path& frag,
			const std::filesystem::path& geom = "");

		void UpdateDescriptorSets(const Shader& shader);

		Buffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

		void CopyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height);

		Image CreateImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageAspectFlags aspects,
			VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, VkSampleCountFlagBits samples);

		void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	private:
		void InitCoreData();
		void SetPhysDevicePropertiesAndLimits();
		void CreateBuffers();
		void CreateSwapchain();
		void GetQueues();
		void CreateDepthResources();
		void CreateGP();
		void CreateBlitPipeline();
		void CreateRenderTextures();
		void CreateCommandPool();
		void CreateCommandBuffers();
		void CreateImmediateCommandResources();
		void CreateSyncObjects();
		void RecreateSwapchain();

		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
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
		Image m_DepthImageMSAA;

		Shader m_GraphicsShader;
		Shader m_WireframeShader;
		Shader m_BlitShader;

		Image m_RenderTexture;
		Image m_RenderTextureResolved;
		VkSampler m_RenderTextureSampler;

		Buffer m_VPBuffer;
		Buffer m_MaterialsBuffer;

		VmaAllocator m_Allocator;

		VkClearColorValue m_ClearColor = {0.0f, 0.0f, 0.0f, 1.0f};

		std::filesystem::path m_ShaderDirectory = std::filesystem::path(PATH_TO_SHADERS);

		VkPhysicalDeviceProperties m_PhysDeviceProperties;
		VkPhysicalDeviceLimits m_PhysDeviceLimits;
		VkSampleCountFlagBits m_MSAASamples = VK_SAMPLE_COUNT_4_BIT;

		std::unordered_map<TimestampType, Timestamp> m_Timestamps;
	};
}

