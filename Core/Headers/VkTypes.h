#pragma once

#include <vector>

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>

#include "Types.h"

namespace Core
{
	struct CoreData
	{
		vkb::Device Device;
		vkb::PhysicalDevice PhysicalDevice;
		vkb::Instance Instance;
		vkb::InstanceDispatchTable DispatchTable;
		vkb::Swapchain Swapchain;
		VkSurfaceKHR Surface;
	};

	struct RenderData
	{
		VkQueue GraphicsQueue;
		VkQueue PresentQueue;
		std::vector<VkImage> SwapchainImages;
		std::vector<VkImageView> SwapchainImageViews;
		std::vector<VkFramebuffer> Framebuffers;

		VkRenderPass RenderPass;
		VkPipeline GraphicsPipeline;
		VkPipelineLayout PipelineLayout;

		VkCommandPool CommandPool;
		std::vector<VkCommandBuffer> CommandBuffers;

		std::vector<VkSemaphore> AvailableSemaphores;
		std::vector<VkSemaphore> FinishedSemaphores;
		std::vector<VkFence> InFlightFences;
		std::vector<VkFence> ImagesInFlight;

		usize CurrentFrame = 0;
	};
}
