#pragma once

#include <vector>

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

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
		u32 QueueFamily;

		std::vector<VkImage> SwapchainImages;
		std::vector<VkImageView> SwapchainImageViews;
		std::vector<VkFramebuffer> Framebuffers;

		VkRenderPass RenderPass;

		VkCommandPool CommandPool;
		std::vector<VkCommandBuffer> CommandBuffers;

		std::vector<VkSemaphore> AvailableSemaphores;
		std::vector<VkSemaphore> FinishedSemaphores;
		std::vector<VkFence> InFlightFences;
		std::vector<VkFence> ImagesInFlight;

		usize CurrentFrame = 0;
	};

	struct Buffer
	{
		VkBuffer Buffer;
		VmaAllocation Allocation;
	};

	struct Image
	{
		VkImage Image;
		VkImageView View;
		VkExtent3D Extent;
		VkFormat Format;
		VmaAllocation Allocation;
	};

	struct DescriptorBinding
	{
		VkDescriptorType Type;

		union
		{
			VkDescriptorImageInfo ImageInfo;
			VkDescriptorBufferInfo BufferInfo;
		};

		explicit DescriptorBinding(const Image& image, const VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL)
			: Type(type)
		{
			ImageInfo.imageView = image.View;
			ImageInfo.imageLayout = layout;
		}

		explicit DescriptorBinding(const Image& image, VkSampler sampler, const VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL)
			: Type(type)
		{
			ImageInfo.imageView = image.View;
			ImageInfo.imageLayout = layout;
			ImageInfo.sampler = sampler;
		}

		explicit DescriptorBinding(VkDescriptorImageInfo info, const VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			: Type(type)
		{
			ImageInfo = info;
		}

		explicit DescriptorBinding(const Buffer& buffer, usize offset, const VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, const VkDeviceSize size = VK_WHOLE_SIZE)
			: Type(type)
		{
			BufferInfo.buffer = buffer.Buffer;
			BufferInfo.offset = offset;
			BufferInfo.range = size;
		}
	};

	struct Shader
	{
		VkPipeline Pipeline;
		VkPipelineLayout PipelineLayout;
		VkDescriptorSet DescriptorSet;
		VkDescriptorSetLayout DescriptorLayout;
		VkDescriptorPool DescriptorPool;

		std::vector<DescriptorBinding> Bindings;

		void Destroy(const VkDevice& device) const
		{
			vkDestroyPipeline(device, Pipeline, nullptr);
			vkDestroyPipelineLayout(device, PipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, DescriptorLayout, nullptr);
			vkDestroyDescriptorPool(device, DescriptorPool, nullptr);
		}
	};

	struct VP
	{
		glm::mat4 View;
		glm::mat4 Projection;
	};

	// cant use Material from Component.h because it might have extra unwanted (in the shader) members in the future
	struct MaterialUBO
	{
		alignas(16) glm::vec3 Color = glm::vec3(0.0f);
	};

	struct ObjPushConstants
	{
		glm::mat4 Model;
		MaterialUBO Material;
	};

	struct MeshBuffers
	{
		Buffer VertexBuffer;
		Buffer IndexBuffer;
		u32 VertexCount;
		u32 IndexCount;
	};
}
