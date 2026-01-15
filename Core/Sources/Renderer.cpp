#include "Renderer.h"

namespace
{
	std::vector<char> ReadFile(const std::filesystem::path& path)
	{
		std::ifstream file(path, std::ios::ate | std::ios::binary);

		ASSERT(file.is_open());

		usize fileSize = static_cast<usize>(file.tellg());
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

		file.close();
		return buffer;
	}

	VkShaderModule CreateShaderModule(Core::CoreData& coreData, const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const u32*>(code.data());

		VkShaderModule shaderModule;
		vkCreateShaderModule(coreData.Device, &createInfo, nullptr, &shaderModule);

		ASSERT(shaderModule);

		return shaderModule;
	}

	VkFormat FindDepthFormat(vkb::PhysicalDevice physicalDevice)
	{
		std::array<VkFormat, 2> candidates =
		{
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT
		};

		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
			if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				return format;
			}
		}

		LOG_CRITICAL("Failed to find supported depth format!");
		return VK_FORMAT_UNDEFINED;
	}
}

namespace Core
{

	void Renderer::Init(GLFWwindow* window)
	{
		m_Window = window;
		InitCoreData();
		CreateBuffers();
		CreateSwapchain();
		GetQueues();
		CreateDepthResources();
		CreateRP();
		CreateRenderTextures();
		CreateGP();
		CreateBlitPipeline();
		CreateFramebuffers();
		CreateCommandPool();
		CreateCommandBuffers();
		CreateImmediateCommandResources();
		CreateSyncObjects();

		TransitionImageLayout(m_RenderTexture.Image, m_RenderTexture.Format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}

	void Renderer::InitCoreData()
	{
		vkb::InstanceBuilder builder;
		auto instRet = builder
			.request_validation_layers()
			.use_default_debug_messenger()
			.require_api_version(1, 3, 2)
			.build();

		if (!instRet)
		{
			LOG_CRITICAL("Failed to create Vulkan instance: {}", instRet.error().message());
			return;
		}

		m_CoreData.Instance = instRet.value();

		glfwCreateWindowSurface(m_CoreData.Instance, m_Window, nullptr, &m_CoreData.Surface);

		VkPhysicalDeviceVulkan13Features features{};

		features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		features.dynamicRendering = true;
		features.synchronization2 = true;

		VkPhysicalDeviceVulkan12Features features12{};
		features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		features12.bufferDeviceAddress = true;
		features12.descriptorIndexing = true;
		features12.hostQueryReset = true;

		vkb::PhysicalDeviceSelector selector(m_CoreData.Instance);
		vkb::PhysicalDevice physicalDevice =
			selector.set_minimum_version(1, 3)
			.set_surface(m_CoreData.Surface)
			.set_required_features_13(features)
			.set_required_features_12(features12)
			.select()
			.value();

		vkb::DeviceBuilder deviceBuilder(physicalDevice);
		vkb::Device vkbDevice = deviceBuilder.build().value();

		m_CoreData.Device = vkbDevice;
		m_CoreData.DispatchTable = m_CoreData.Instance.make_table();
		m_CoreData.PhysicalDevice = physicalDevice;

		VmaAllocatorCreateInfo allocatorCreateInfo{};
		allocatorCreateInfo.physicalDevice = m_CoreData.PhysicalDevice;
		allocatorCreateInfo.device = m_CoreData.Device;
		allocatorCreateInfo.instance = m_CoreData.Instance;
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

		vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator);
	}
	
	void Renderer::CreateBuffers()
	{
		m_VPBuffer = CreateBuffer(sizeof(VP), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	}

	void Renderer::CreateSwapchain()
	{
		auto swapchain = vkb::SwapchainBuilder(m_CoreData.Device)
			.set_old_swapchain(m_CoreData.Swapchain)
			.build()
			.value();

		vkb::destroy_swapchain(m_CoreData.Swapchain);
		m_CoreData.Swapchain = swapchain;
	}

	void Renderer::GetQueues()
	{
		m_RenderData.GraphicsQueue = m_CoreData.Device.get_queue(vkb::QueueType::graphics).value();
		m_RenderData.PresentQueue = m_CoreData.Device.get_queue(vkb::QueueType::present).value();
		m_RenderData.QueueFamily = m_CoreData.Device.get_queue_index(vkb::QueueType::graphics).value();

		ASSERT(m_RenderData.GraphicsQueue && m_RenderData.PresentQueue);
	}

	void Renderer::CreateDepthResources()
	{
		VkFormat depthFormat = FindDepthFormat(m_CoreData.PhysicalDevice);
		
		m_DepthImage = CreateImage(
			m_CoreData.Swapchain.extent.width,
			m_CoreData.Swapchain.extent.height,
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);
	}

	void Renderer::CreateRP()
	{
		std::array<VkAttachmentDescription, 2> attachments = {};

		attachments[0].format = m_CoreData.Swapchain.image_format;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		attachments[1].format = m_DepthImage.Format;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo rpCreateInfo = {};
		rpCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rpCreateInfo.attachmentCount = static_cast<u32>(attachments.size());
		rpCreateInfo.pAttachments = attachments.data();
		rpCreateInfo.subpassCount = 1;
		rpCreateInfo.pSubpasses = &subpass;
		rpCreateInfo.dependencyCount = 1;
		rpCreateInfo.pDependencies = &dependency;

		vkCreateRenderPass(m_CoreData.Device, &rpCreateInfo, nullptr, &m_RenderData.RenderPass);

		attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		vkCreateRenderPass(m_CoreData.Device, &rpCreateInfo, nullptr, &m_RenderTextureRenderPass);

		ASSERT(m_RenderData.RenderPass);
		ASSERT(m_RenderTextureRenderPass);
	}

	void Renderer::CreateGP()
	{
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<f32>(m_CoreData.Swapchain.extent.width);
		viewport.height = static_cast<f32>(m_CoreData.Swapchain.extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = { m_CoreData.Swapchain.extent.width, m_CoreData.Swapchain.extent.height };

		VkPushConstantRange objPushConstantRange = {};
		objPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		objPushConstantRange.offset = 0;
		objPushConstantRange.size = sizeof(ObjPushConstants);

		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		attributeDescriptions.resize(3);

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, Position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, Normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, TextureCoordinate);

		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_TRUE;
		depthStencil.front.passOp = VK_STENCIL_OP_REPLACE;
		depthStencil.front.compareOp = VK_COMPARE_OP_ALWAYS;
		depthStencil.front.reference = 1;
		depthStencil.front.compareMask = 0xFF;
		depthStencil.front.writeMask = 0xFF;
		depthStencil.back = depthStencil.front;

		std::vector<DescriptorBinding> bindings =
		{
			DescriptorBinding(m_VPBuffer, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		};

		auto vert = m_ShaderDirectory / "Compiled" / "object.vert.spv";
		auto frag = m_ShaderDirectory / "Compiled" / "object.frag.spv";

		m_GraphicsShader = CreateShader(m_RenderTextureRenderPass, bindings, { objPushConstantRange },
			&bindingDescription, attributeDescriptions, &viewport, &scissor, &depthStencil, VK_CULL_MODE_BACK_BIT, vert, frag);
		UpdateDescriptorSets(m_GraphicsShader);
	}

	void Renderer::CreateBlitPipeline()
	{

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<f32>(m_CoreData.Swapchain.extent.width);
		viewport.height = static_cast<f32>(m_CoreData.Swapchain.extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = m_CoreData.Swapchain.extent;

		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		std::vector<DescriptorBinding> bindings =
		{
			DescriptorBinding(m_RenderTexture, m_RenderTextureSampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		};

		auto vert = m_ShaderDirectory / "Compiled" / "blit.vert.spv";
		auto frag = m_ShaderDirectory / "Compiled" / "blit.frag.spv";

		m_BlitShader = CreateShader(m_RenderData.RenderPass, bindings, {}, nullptr, {}, &viewport, &scissor, &depthStencil, VK_CULL_MODE_BACK_BIT, vert, frag);
		UpdateDescriptorSets(m_BlitShader);
	}

	void Renderer::CreateRenderTextures()
	{
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		vkCreateSampler(m_CoreData.Device, &samplerInfo, nullptr, &m_RenderTextureSampler);
		ASSERT(m_RenderTextureSampler);


		m_RenderTexture = CreateImage(m_CoreData.Swapchain.extent.width, m_CoreData.Swapchain.extent.height,
			VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

		std::array attachments = 
		{
			m_RenderTexture.View,
			m_DepthImage.View
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_RenderTextureRenderPass;
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_CoreData.Swapchain.extent.width;
		framebufferInfo.height = m_CoreData.Swapchain.extent.height;
		framebufferInfo.layers = 1;

		vkCreateFramebuffer(m_CoreData.Device, &framebufferInfo, nullptr, &m_RenderTextureFramebuffer);
		ASSERT(m_RenderTextureFramebuffer);
	}

	void Renderer::CreateFramebuffers()
	{
		m_RenderData.SwapchainImages = m_CoreData.Swapchain.get_images().value();
		m_RenderData.SwapchainImageViews = m_CoreData.Swapchain.get_image_views().value();

		m_RenderData.Framebuffers.resize(m_RenderData.SwapchainImageViews.size());

		for (usize i = 0; i < m_RenderData.SwapchainImageViews.size(); i++)
		{
			std::array attachments = 
			{ 
				m_RenderData.SwapchainImageViews[i],
				m_DepthImage.View
			};

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderData.RenderPass;
			framebufferInfo.attachmentCount = static_cast<u32>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = m_CoreData.Swapchain.extent.width;
			framebufferInfo.height = m_CoreData.Swapchain.extent.height;
			framebufferInfo.layers = 1;

			vkCreateFramebuffer(m_CoreData.Device, &framebufferInfo, nullptr, &m_RenderData.Framebuffers[i]);
			ASSERT(m_RenderData.Framebuffers[i]);
		}
	}

	void Renderer::CreateCommandPool()
	{
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = m_CoreData.Device.get_queue_index(vkb::QueueType::graphics).value();
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		vkCreateCommandPool(m_CoreData.Device, &poolInfo, nullptr, &m_RenderData.CommandPool);
		ASSERT(m_RenderData.CommandPool);
	}

	void Renderer::CreateCommandBuffers()
	{
		m_RenderData.CommandBuffers.resize(m_RenderData.Framebuffers.size());

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_RenderData.CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<u32>(m_RenderData.CommandBuffers.size());

		vkAllocateCommandBuffers(m_CoreData.Device, &allocInfo, m_RenderData.CommandBuffers.data());
	}

	void Renderer::CreateImmediateCommandResources()
	{
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = m_CoreData.Device.get_queue_index(vkb::QueueType::graphics).value();
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		vkCreateCommandPool(m_CoreData.Device, &poolInfo, nullptr, &m_ImmediateCommandPool);
		ASSERT(m_ImmediateCommandPool);

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_ImmediateCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers(m_CoreData.Device, &allocInfo, &m_ImmediateCommandBuffer);
		ASSERT(m_ImmediateCommandBuffer);
	}

	void Renderer::CreateSyncObjects()
	{
		m_RenderData.AvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_RenderData.FinishedSemaphores.resize(m_CoreData.Swapchain.image_count);
		m_RenderData.InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		m_RenderData.ImagesInFlight.resize(m_CoreData.Swapchain.image_count, VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (usize i = 0; i < m_CoreData.Swapchain.image_count; i++)
		{
			vkCreateSemaphore(m_CoreData.Device, &semaphoreInfo, nullptr, &m_RenderData.FinishedSemaphores[i]);
		}

		for (usize i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkCreateSemaphore(m_CoreData.Device, &semaphoreInfo, nullptr, &m_RenderData.AvailableSemaphores[i]);
			vkCreateFence(m_CoreData.Device, &fenceInfo, nullptr, &m_RenderData.InFlightFences[i]);
		}
	}

	Buffer Renderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = memoryUsage;

		Buffer buffer;
		VkResult result = vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, &buffer.Buffer, &buffer.Allocation, nullptr);
		ASSERT(result == VK_SUCCESS);

		return buffer;
	}

	void Renderer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		ImmediateSubmit([&](VkCommandBuffer cmd)
			{
				VkBufferCopy copyRegion = {};
				copyRegion.size = size;
				vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);
			});
	}

	Image Renderer::CreateImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageAspectFlags aspects, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage)
	{
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = memoryUsage;

		Image image;
		image.Format = format;
		image.Extent = { width, height, 1 };

		vmaCreateImage(m_Allocator, &imageInfo, &allocInfo, &image.Image, &image.Allocation, nullptr);

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image.Image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspects;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		vkCreateImageView(m_CoreData.Device, &viewInfo, nullptr, &image.View);

		return image;
	}

	Shader Renderer::CreateShader(const VkRenderPass& renderPass, const std::vector<DescriptorBinding>& bindings,
		const std::vector<VkPushConstantRange>& pushConstantRange,
		VkVertexInputBindingDescription* vtxInputBindingDesc,
		std::vector<VkVertexInputAttributeDescription> vtxInputAttrDesc,
		VkViewport* viewport, VkRect2D* scissor,
		VkPipelineDepthStencilStateCreateInfo* depthStencilInfo,
		VkCullModeFlagBits cullMode,
		const std::filesystem::path& vert, const std::filesystem::path& frag)
	{
		Shader shader;
		shader.Bindings = bindings;

		std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
		std::unordered_map<VkDescriptorType, uint32_t> descriptorCounts;

		for (size_t i = 0; i < bindings.size(); ++i)
		{
			layoutBindings.emplace_back(
				static_cast<uint32_t>(i),
				bindings[i].Type,
				1,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
			);
			descriptorCounts[bindings[i].Type]++;
		}

		std::vector<VkDescriptorPoolSize> poolSizes;
		for (const auto& [type, count] : descriptorCounts)
		{
			poolSizes.emplace_back(type, count);
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = layoutBindings.data();

		vkCreateDescriptorSetLayout(m_CoreData.Device, &layoutInfo, nullptr, &shader.DescriptorLayout);
		ASSERT(shader.DescriptorLayout);

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 1;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();

		vkCreateDescriptorPool(m_CoreData.Device, &poolInfo, nullptr, &shader.DescriptorPool);
		ASSERT(shader.DescriptorPool);

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = shader.DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &shader.DescriptorLayout;

		vkAllocateDescriptorSets(m_CoreData.Device, &allocInfo, &shader.DescriptorSet);
		ASSERT(shader.DescriptorSet);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &shader.DescriptorLayout;
		pipelineLayoutInfo.pPushConstantRanges = !pushConstantRange.empty() ? pushConstantRange.data() : nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = static_cast<u32>(pushConstantRange.size());

		vkCreatePipelineLayout(m_CoreData.Device, &pipelineLayoutInfo, nullptr, &shader.PipelineLayout);
		ASSERT(shader.PipelineLayout);

		auto vertCode = ReadFile(vert);
		auto fragCode = ReadFile(frag);

		VkShaderModule vertModule = CreateShaderModule(m_CoreData, vertCode);
		VkShaderModule fragModule = CreateShaderModule(m_CoreData, fragCode);

		ASSERT(vertModule && fragModule);

		VkPipelineShaderStageCreateInfo vertStageInfo = {};
		vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertStageInfo.module = vertModule;
		vertStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragStageInfo = {};
		fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragStageInfo.module = fragModule;
		fragStageInfo.pName = "main";

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = vtxInputBindingDesc ? 1 : 0;
		vertexInputInfo.pVertexBindingDescriptions = vtxInputBindingDesc ? vtxInputBindingDesc : nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = vtxInputAttrDesc.size();
		vertexInputInfo.pVertexAttributeDescriptions = !vtxInputAttrDesc.empty() ? vtxInputAttrDesc.data() : nullptr;
		
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = cullMode;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		std::array dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		std::array shaderStages = { vertStageInfo, fragStageInfo };

		VkPipelineDynamicStateCreateInfo dynamicInfo = {};
		dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicInfo.dynamicStateCount = static_cast<u32>(dynamicStates.size());
		dynamicInfo.pDynamicStates = dynamicStates.data();

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pDepthStencilState = depthStencilInfo;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicInfo;
		pipelineInfo.layout = shader.PipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		vkCreateGraphicsPipelines(m_CoreData.Device, nullptr, 1, &pipelineInfo, nullptr, &shader.Pipeline);
		ASSERT(shader.Pipeline);

		vkDestroyShaderModule(m_CoreData.Device, fragModule, nullptr);
		vkDestroyShaderModule(m_CoreData.Device, vertModule, nullptr);

		return shader;
	}

	void Renderer::UpdateDescriptorSets(const Shader& shader)
	{
		std::vector<VkWriteDescriptorSet> descriptorWrites;

		for (usize i = 0; i < shader.Bindings.size(); ++i)
		{
			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = shader.DescriptorSet;
			writeDescriptorSet.dstBinding = static_cast<u32>(i);
			writeDescriptorSet.descriptorType = shader.Bindings[i].Type;
			writeDescriptorSet.descriptorCount = 1;

			if (shader.Bindings[i].Type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
				shader.Bindings[i].Type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
				shader.Bindings[i].Type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				writeDescriptorSet.pImageInfo = &shader.Bindings[i].ImageInfo;
			}
			else
			{
				writeDescriptorSet.pBufferInfo = &shader.Bindings[i].BufferInfo;
			}

			descriptorWrites.push_back(writeDescriptorSet);
		}

		vkUpdateDescriptorSets(m_CoreData.Device, static_cast<u32>(descriptorWrites.size()),
			descriptorWrites.data(), 0, nullptr);
	}

	void Renderer::RecreateSwapchain()
	{
		vkDeviceWaitIdle(m_CoreData.Device);

		for (auto framebuffer : m_RenderData.Framebuffers)
		{
			vkDestroyFramebuffer(m_CoreData.Device, framebuffer, nullptr);
		}
		m_RenderData.Framebuffers.clear();

		for (auto imageView : m_RenderData.SwapchainImageViews)
		{
			vkDestroyImageView(m_CoreData.Device, imageView, nullptr);
		}
		m_RenderData.SwapchainImageViews.clear();

		vkDestroyFramebuffer(m_CoreData.Device, m_RenderTextureFramebuffer, nullptr);
		vkDestroyImageView(m_CoreData.Device, m_RenderTexture.View, nullptr);
		vmaDestroyImage(m_Allocator, m_RenderTexture.Image, m_RenderTexture.Allocation);

		vkDestroyImageView(m_CoreData.Device, m_DepthImage.View, nullptr);
		vmaDestroyImage(m_Allocator, m_DepthImage.Image, m_DepthImage.Allocation);

		vkFreeCommandBuffers(m_CoreData.Device, m_RenderData.CommandPool,
			static_cast<uint32_t>(m_RenderData.CommandBuffers.size()),
			m_RenderData.CommandBuffers.data());
		m_RenderData.CommandBuffers.clear();

		vkDestroyCommandPool(m_CoreData.Device, m_RenderData.CommandPool, nullptr);

		m_GraphicsShader.Destroy(m_CoreData.Device);
		m_BlitShader.Destroy(m_CoreData.Device);

		vkDestroyRenderPass(m_CoreData.Device, m_RenderData.RenderPass, nullptr);
		vkDestroyRenderPass(m_CoreData.Device, m_RenderTextureRenderPass, nullptr);

		vkDestroySampler(m_CoreData.Device, m_RenderTextureSampler, nullptr);

		CreateSwapchain();
		GetQueues();
		CreateDepthResources();
		CreateRP();
		CreateRenderTextures();
		CreateGP();
		CreateBlitPipeline();
		CreateFramebuffers();
		CreateCommandPool();
		CreateCommandBuffers();

		TransitionImageLayout(m_RenderTexture.Image, m_RenderTexture.Format,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}

	void Renderer::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(m_ImmediateCommandBuffer, &beginInfo);

		function(m_ImmediateCommandBuffer);

		vkEndCommandBuffer(m_ImmediateCommandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_ImmediateCommandBuffer;

		vkQueueSubmit(m_RenderData.GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_RenderData.GraphicsQueue);

	}

	void Renderer::BeginFrame()
	{
		vkWaitForFences(m_CoreData.Device, 1, &m_RenderData.InFlightFences[m_RenderData.CurrentFrame], VK_TRUE, UINT64_MAX);

		VkResult result = vkAcquireNextImageKHR(
			m_CoreData.Device, m_CoreData.Swapchain, UINT64_MAX,
			m_RenderData.AvailableSemaphores[m_RenderData.CurrentFrame],
			VK_NULL_HANDLE, &m_CurrentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapchain();
			m_FrameInProgress = false;
			return;
		}
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			LOG_CRITICAL("Failed to acquire swapchain image!: {}", static_cast<u32>(result));
			m_FrameInProgress = false;
			return;
		}

		if (m_RenderData.ImagesInFlight[m_CurrentImageIndex] != VK_NULL_HANDLE)
		{
			vkWaitForFences(m_CoreData.Device, 1, &m_RenderData.ImagesInFlight[m_CurrentImageIndex], VK_TRUE, UINT64_MAX);
		}
		m_RenderData.ImagesInFlight[m_CurrentImageIndex] = m_RenderData.InFlightFences[m_RenderData.CurrentFrame];

		m_CurrentCommandBuffer = m_RenderData.CommandBuffers[m_CurrentImageIndex];
		vkResetCommandBuffer(m_CurrentCommandBuffer, 0);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(m_CurrentCommandBuffer, &beginInfo);

		m_FrameInProgress = true;
	}

	void Renderer::EndFrame()
	{
		if (!m_FrameInProgress)
		{
			LOG_WARN("EndFrame called without frame in progress.");
			return;
		}

		vkEndCommandBuffer(m_CurrentCommandBuffer);

		std::array waitSemaphores = { m_RenderData.AvailableSemaphores[m_RenderData.CurrentFrame] };
		std::array waitStages = { static_cast<VkPipelineStageFlags>(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT) };

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores.data();
		submitInfo.pWaitDstStageMask = waitStages.data();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CurrentCommandBuffer;

		std::array signalSemaphores = { m_RenderData.FinishedSemaphores[m_CurrentImageIndex] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores.data();

		vkResetFences(m_CoreData.Device, 1, &m_RenderData.InFlightFences[m_RenderData.CurrentFrame]);
		vkQueueSubmit(m_RenderData.GraphicsQueue, 1, &submitInfo, m_RenderData.InFlightFences[m_RenderData.CurrentFrame]);

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores.data();

		std::array swapchains = { static_cast<VkSwapchainKHR>(m_CoreData.Swapchain) };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains.data();
		presentInfo.pImageIndices = &m_CurrentImageIndex;

		VkResult result = vkQueuePresentKHR(m_RenderData.PresentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			RecreateSwapchain();
		}
		else if (result != VK_SUCCESS)
		{
			LOG_CRITICAL("Failed to present swapchain image!");
		}

		m_RenderData.CurrentFrame = (m_RenderData.CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		m_FrameInProgress = false;
	}

	void Renderer::BeginRenderToTexture()
	{
		if (!m_FrameInProgress)
		{
			LOG_WARN("BeginRenderToTexture called without frame in progress.");
			return;
		}

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { m_ClearColor };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo rpInfo = {};
		rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rpInfo.renderPass = m_RenderTextureRenderPass;
		rpInfo.framebuffer = m_RenderTextureFramebuffer;
		rpInfo.renderArea.offset = { 0, 0 };
		rpInfo.renderArea.extent = { m_CoreData.Swapchain.extent.width, m_CoreData.Swapchain.extent.height };
		rpInfo.clearValueCount = static_cast<u32>(clearValues.size());
		rpInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_CurrentCommandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(m_CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsShader.Pipeline);
		vkCmdBindDescriptorSets(m_CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_GraphicsShader.PipelineLayout, 0, 1, &m_GraphicsShader.DescriptorSet, 0, nullptr);

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<f32>(m_CoreData.Swapchain.extent.width);
		viewport.height = static_cast<f32>(m_CoreData.Swapchain.extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(m_CurrentCommandBuffer, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = { m_CoreData.Swapchain.extent.width, m_CoreData.Swapchain.extent.height };
		vkCmdSetScissor(m_CurrentCommandBuffer, 0, 1, &scissor);
	}

	void Renderer::EndRenderToTexture()
	{
		if (!m_FrameInProgress)
		{
			LOG_WARN("EndRenderToTexture called without frame in progress.");
			return;
		}

		vkCmdEndRenderPass(m_CurrentCommandBuffer);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_RenderTexture.Image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			m_CurrentCommandBuffer,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void Renderer::BeginRenderToSwapchain()
	{
		if (!m_FrameInProgress)
		{
			LOG_WARN("BeginRenderToSwapchain called without frame in progress.");
			return;
		}

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo rpInfo = {};
		rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rpInfo.renderPass = m_RenderData.RenderPass;
		rpInfo.framebuffer = m_RenderData.Framebuffers[m_CurrentImageIndex];
		rpInfo.renderArea.offset = { 0, 0 };
		rpInfo.renderArea.extent = m_CoreData.Swapchain.extent;
		rpInfo.clearValueCount = static_cast<u32>(clearValues.size());
		rpInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_CurrentCommandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<f32>(m_CoreData.Swapchain.extent.width);
		viewport.height = static_cast<f32>(m_CoreData.Swapchain.extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(m_CurrentCommandBuffer, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = m_CoreData.Swapchain.extent;
		vkCmdSetScissor(m_CurrentCommandBuffer, 0, 1, &scissor);

		vkCmdBindPipeline(m_CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_BlitShader.Pipeline);
		vkCmdBindDescriptorSets(m_CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_BlitShader.PipelineLayout, 0, 1, &m_BlitShader.DescriptorSet, 0, nullptr);

		vkCmdDraw(m_CurrentCommandBuffer, 3, 1, 0, 0);
	}

	void Renderer::EndRenderToSwapchain()
	{
		if (!m_FrameInProgress)
		{
			LOG_WARN("EndRenderToSwapchain called without frame in progress.");
			return;
		}

		vkCmdEndRenderPass(m_CurrentCommandBuffer);
	}

	MeshBuffers Renderer::CreateMeshBuffers(const std::vector<Vertex>& vertices, const std::vector<u32>& indices)
	{
		MeshBuffers meshBuffers;
		meshBuffers.VertexCount = static_cast<u32>(vertices.size());
		meshBuffers.IndexCount = static_cast<u32>(indices.size());

		VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();

		Buffer stagingBuffer = CreateBuffer(
			vertexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY
		);

		void* data;
		vmaMapMemory(m_Allocator, stagingBuffer.Allocation, &data);
		std::memcpy(data, vertices.data(), vertexBufferSize);
		vmaUnmapMemory(m_Allocator, stagingBuffer.Allocation);

		meshBuffers.VertexBuffer = CreateBuffer(
			vertexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		CopyBuffer(stagingBuffer.Buffer, meshBuffers.VertexBuffer.Buffer, vertexBufferSize);
		vmaDestroyBuffer(m_Allocator, stagingBuffer.Buffer, stagingBuffer.Allocation);

		VkDeviceSize indexBufferSize = sizeof(u32) * indices.size();

		stagingBuffer = CreateBuffer(
			indexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY
		);

		vmaMapMemory(m_Allocator, stagingBuffer.Allocation, &data);
		std::memcpy(data, indices.data(), indexBufferSize);
		vmaUnmapMemory(m_Allocator, stagingBuffer.Allocation);

		meshBuffers.IndexBuffer = CreateBuffer(
			indexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		CopyBuffer(stagingBuffer.Buffer, meshBuffers.IndexBuffer.Buffer, indexBufferSize);
		vmaDestroyBuffer(m_Allocator, stagingBuffer.Buffer, stagingBuffer.Allocation);

		return meshBuffers;
	}

	void Renderer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		ImmediateSubmit([&](VkCommandBuffer cmd)
			{
				VkImageMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.oldLayout = oldLayout;
				barrier.newLayout = newLayout;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = image;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;

				if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

					if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
					{
						barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}

				VkPipelineStageFlags sourceStage;
				VkPipelineStageFlags destinationStage;

				if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
				{
					barrier.srcAccessMask = 0;
					barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				}
				else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				{
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
					destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				}
				else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
				{
					barrier.srcAccessMask = 0;
					barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
				}
				else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
				{
					barrier.srcAccessMask = 0;
					barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				}
				else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				{
					barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
					destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				}
				else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
				{
					barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
					barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
					destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				}
				else
				{
					LOG_CRITICAL("Unsupported layout transition!");
					return;
				}

				vkCmdPipelineBarrier(
					cmd,
					sourceStage, destinationStage,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);
			});
	}

	void Renderer::Cleanup()
	{
		vkDeviceWaitIdle(m_CoreData.Device);

		for (usize i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(m_CoreData.Device, m_RenderData.AvailableSemaphores[i], nullptr);
			vkDestroyFence(m_CoreData.Device, m_RenderData.InFlightFences[i], nullptr);
		}

		for (usize i = 0; i < m_RenderData.FinishedSemaphores.size(); i++)
		{
			vkDestroySemaphore(m_CoreData.Device, m_RenderData.FinishedSemaphores[i], nullptr);
		}

		vkDestroyCommandPool(m_CoreData.Device, m_RenderData.CommandPool, nullptr);
		vkDestroyCommandPool(m_CoreData.Device, m_ImmediateCommandPool, nullptr);

		for (auto framebuffer : m_RenderData.Framebuffers)
		{
			vkDestroyFramebuffer(m_CoreData.Device, framebuffer, nullptr);
		}
		vkDestroyFramebuffer(m_CoreData.Device, m_RenderTextureFramebuffer, nullptr);

		m_GraphicsShader.Destroy(m_CoreData.Device);
		m_BlitShader.Destroy(m_CoreData.Device);

		vkDestroyRenderPass(m_CoreData.Device, m_RenderData.RenderPass, nullptr);
		vkDestroyRenderPass(m_CoreData.Device, m_RenderTextureRenderPass, nullptr);

		for (auto imageView : m_RenderData.SwapchainImageViews)
		{
			vkDestroyImageView(m_CoreData.Device, imageView, nullptr);
		}
		vkDestroyImageView(m_CoreData.Device, m_RenderTexture.View, nullptr);
		vkDestroyImageView(m_CoreData.Device, m_DepthImage.View, nullptr);

		vkDestroySampler(m_CoreData.Device, m_RenderTextureSampler, nullptr);

		vmaDestroyBuffer(m_Allocator, m_VPBuffer.Buffer, m_VPBuffer.Allocation);

		vmaDestroyImage(m_Allocator, m_RenderTexture.Image, m_RenderTexture.Allocation);
		vmaDestroyImage(m_Allocator, m_DepthImage.Image, m_DepthImage.Allocation);
		
		vkb::destroy_swapchain(m_CoreData.Swapchain);

		vmaDestroyAllocator(m_Allocator);

		vkDestroySurfaceKHR(m_CoreData.Instance, m_CoreData.Surface, nullptr);
		vkb::destroy_device(m_CoreData.Device);
		vkb::destroy_instance(m_CoreData.Instance);
	}
}