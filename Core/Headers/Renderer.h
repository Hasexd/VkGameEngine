#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include <array>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include "Log.h"
#include "Types.h"
#include "VkTypes.h"
#include "Mesh.h"
#include "Transform.h"
#include "Camera.h"

constexpr usize MAX_FRAMES_IN_FLIGHT = 2;

namespace Core
{
	class Renderer
	{
	public:
		Renderer() = default;

		void Init(GLFWwindow* window);
		void DrawFrame();
		void Cleanup();

		[[nodiscard]] vkb::Device GetDevice() const noexcept { return m_CoreData.Device; }

	private:
		void InitCoreData();
		void CreateSwapchain();
		void GetQueues();
		void CreateDepthResources();
		void CreateRP();
		void CreateGP();
		void CreateFramebuffers();
		void CreateCommandPool();
		void CreateCommandBuffers();
		void CreateSyncObjects();
		void RecreateSwapchain();

		Image CreateImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling,
			VkImageUsageFlags usage, VmaMemoryUsage memoryUsage);

	private:
		GLFWwindow* m_Window;

		CoreData m_CoreData;
		RenderData m_RenderData;

		Image m_DepthImage;
		Shader m_GraphicsShader;
		Camera m_Camera;

		Mesh m_CubeMesh;
		Transform m_CubeTransform;

		VmaAllocator m_Allocator;
	};
}

