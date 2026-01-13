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

		VkCommandBuffer GetCurrentCommandBuffer() const { return m_CurrentCommandBuffer; }
		VkPipelineLayout GetGraphicsPipelineLayout() const { return m_GraphicsShader.PipelineLayout; }

		[[nodiscard]] vkb::Device GetDevice() const noexcept { return m_CoreData.Device; }

		MeshBuffers CreateMeshBuffers(const std::vector<Vertex>& vertices, const std::vector<u32>& indices);
	private:
		void InitCoreData();
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

		Buffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

		Image CreateImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageAspectFlags aspects,
			VkImageUsageFlags usage, VmaMemoryUsage memoryUsage);

		void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

		Shader CreateShader(const VkRenderPass& renderPass, const std::vector<DescriptorBinding>& bindings,
			const std::vector<VkPushConstantRange>& pushConstantRanges,
			VkVertexInputBindingDescription* vtxInputBindingDesc,
			std::vector<VkVertexInputAttributeDescription> vtxInputAttrDesc,
			VkViewport* viewport,
			VkRect2D* scissor,
			const std::string& name);

		void UpdateDescriptorSets(const Shader& shader);

	private:
		GLFWwindow* m_Window;

		CoreData m_CoreData;
		RenderData m_RenderData;

		VkCommandBuffer m_CurrentCommandBuffer;
		u32 m_CurrentImageIndex;

		bool m_FrameInProgress = false;

		VkCommandPool m_ImmediateCommandPool;
		VkCommandBuffer m_ImmediateCommandBuffer;

		Image m_DepthImage;

		Shader m_GraphicsShader;
		Shader m_BlitShader;

		Image m_RenderTexture;
		VkSampler m_RenderTextureSampler;
		VkRenderPass m_RenderTextureRenderPass;
		VkFramebuffer m_RenderTextureFramebuffer;
		u32 m_RenderTextureWidth;
		u32 m_RenderTextureHeight;

		VmaAllocator m_Allocator;
	};
}

