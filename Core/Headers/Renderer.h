#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <vector>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>

#include <vk_mem_alloc.h>

#include "Log.h"
#include "Types.h"
#include "VkTypes.h"

constexpr usize MAX_FRAMES_IN_FLIGHT = 2;

namespace Core
{
	class Renderer
	{
	public:
		Renderer() = default;

		void Init(GLFWwindow* window);
		void DrawFrame();

		[[nodiscard]] vkb::Device GetDevice() const noexcept { return m_CoreData.Device; }

	private:
		void InitCoreData();
		void CreateSwapchain();
		void GetQueues();
		void CreateRP();
		void CreateGP();
		void CreateFramebuffers();
		void CreateCommandPool();
		void CreateCommandBuffers();
		void CreateSyncObjects();
		void RecreateSwapchain();

	private:
		GLFWwindow* m_Window;

		CoreData m_CoreData;
		RenderData m_RenderData;

		VmaAllocator m_Allocator;
	};
}

