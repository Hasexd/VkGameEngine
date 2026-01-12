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
		std::array<VkFormat, 3> candidates =
		{
			VK_FORMAT_D32_SFLOAT,
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
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
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
	}

	void Renderer::CreateGP()
	{
		std::filesystem::path pathToCompiled = std::filesystem::path(PATH_TO_SHADERS) / "Compiled";
		auto vertCode = ReadFile(pathToCompiled / "object.vert.spv");
		auto fragCode = ReadFile(pathToCompiled / "object.frag.spv");

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


		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(f32) * 6;
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = 0;

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = sizeof(f32) * 3;

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = 2;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<f32>(m_RenderTextureWidth);
		viewport.height = static_cast<f32>(m_RenderTextureHeight);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = { m_RenderTextureWidth, m_RenderTextureHeight };

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

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

		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(MVP);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		vkCreatePipelineLayout(m_CoreData.Device, &pipelineLayoutInfo, nullptr, &m_GraphicsShader.PipelineLayout);
		ASSERT(m_GraphicsShader.PipelineLayout);

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
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicInfo;
		pipelineInfo.layout = m_GraphicsShader.PipelineLayout;
		pipelineInfo.renderPass = m_RenderTextureRenderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		vkCreateGraphicsPipelines(m_CoreData.Device, nullptr, 1, &pipelineInfo, nullptr, &m_GraphicsShader.Pipeline);
		ASSERT(m_GraphicsShader.Pipeline);

		vkDestroyShaderModule(m_CoreData.Device, fragModule, nullptr);
		vkDestroyShaderModule(m_CoreData.Device, vertModule, nullptr);
	}

	void Renderer::CreateBlitPipeline()
	{
		std::filesystem::path pathToCompiled = std::filesystem::path(PATH_TO_SHADERS) / "Compiled";
		auto vertCode = ReadFile(pathToCompiled / "blit.vert.spv");
		auto fragCode = ReadFile(pathToCompiled / "blit.frag.spv");

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
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

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

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_FALSE;
		depthStencil.depthWriteEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

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
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

		VkDescriptorSetLayoutBinding samplerBinding = {};
		samplerBinding.binding = 0;
		samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerBinding.descriptorCount = 1;
		samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &samplerBinding;

		vkCreateDescriptorSetLayout(m_CoreData.Device, &layoutInfo, nullptr, &m_BlitShader.DescriptorLayout);
		ASSERT(m_BlitShader.DescriptorLayout);

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

		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize.descriptorCount = 1;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = 1;

		vkCreateDescriptorPool(m_CoreData.Device, &poolInfo, nullptr, &m_BlitShader.DescriptorPool);
		ASSERT(m_BlitShader.DescriptorPool);

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_BlitShader.DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &m_BlitShader.DescriptorLayout;

		vkAllocateDescriptorSets(m_CoreData.Device, &allocInfo, &m_BlitShader.DescriptorSet);
		ASSERT(m_BlitShader.DescriptorSet);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_RenderTexture.View;
		imageInfo.sampler = m_RenderTextureSampler;

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_BlitShader.DescriptorSet;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(m_CoreData.Device, 1, &descriptorWrite, 0, nullptr);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &m_BlitShader.DescriptorLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 0;

		vkCreatePipelineLayout(m_CoreData.Device, &pipelineLayoutInfo, nullptr, &m_BlitShader.PipelineLayout);
		ASSERT(m_BlitShader.PipelineLayout);

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
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicInfo;
		pipelineInfo.layout = m_BlitShader.PipelineLayout;
		pipelineInfo.renderPass = m_RenderData.RenderPass;
		pipelineInfo.subpass = 0;

		vkCreateGraphicsPipelines(m_CoreData.Device, nullptr, 1, &pipelineInfo, nullptr, &m_BlitShader.Pipeline);
		ASSERT(m_BlitShader.Pipeline);

		vkDestroyShaderModule(m_CoreData.Device, fragModule, nullptr);
		vkDestroyShaderModule(m_CoreData.Device, vertModule, nullptr);
	}

	void Renderer::CreateRenderTextures()
	{
		m_RenderTexture = CreateImage(m_CoreData.Swapchain.extent.width, m_CoreData.Swapchain.extent.height,
			VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

		std::array attachments = 
		{
			m_RenderTexture.View,
			m_DepthImage.View
		};

		m_RenderTextureWidth = m_CoreData.Swapchain.extent.width;
		m_RenderTextureHeight = m_CoreData.Swapchain.extent.height;

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_RenderData.RenderPass;
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_RenderTextureWidth;
		framebufferInfo.height = m_RenderTextureHeight;
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

	void Renderer::RecreateSwapchain()
	{
		vkDeviceWaitIdle(m_CoreData.Device);

		vkDestroyImageView(m_CoreData.Device, m_DepthImage.View, nullptr);
		vmaDestroyImage(m_Allocator, m_DepthImage.Image, m_DepthImage.Allocation);

		vkDestroyCommandPool(m_CoreData.Device, m_RenderData.CommandPool, nullptr);

		for (auto framebuffer : m_RenderData.Framebuffers)
		{
			vkDestroyFramebuffer(m_CoreData.Device, framebuffer, nullptr);
		}

		for (auto imageView : m_RenderData.SwapchainImageViews)
		{
			vkDestroyImageView(m_CoreData.Device, imageView, nullptr);
		}

		CreateSwapchain();
		CreateDepthResources();
		CreateRenderTextures();
		CreateFramebuffers();
		CreateCommandPool();
		CreateCommandBuffers();
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

	void Renderer::DrawFrame(const std::vector<std::shared_ptr<Object>>& objects, const Camera& camera)
	{
		vkWaitForFences(m_CoreData.Device, 1, &m_RenderData.InFlightFences[m_RenderData.CurrentFrame], VK_TRUE, UINT64_MAX);

		u32 imageIndex = 0;
		VkResult result = vkAcquireNextImageKHR(
			m_CoreData.Device, m_CoreData.Swapchain, UINT64_MAX, m_RenderData.AvailableSemaphores[m_RenderData.CurrentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapchain();
			return;
		}
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			LOG_CRITICAL("Failed to acquire swapchain image!");
			return;
		}

		if (m_RenderData.ImagesInFlight[imageIndex] != VK_NULL_HANDLE)
		{
			vkWaitForFences(m_CoreData.Device, 1, &m_RenderData.ImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
		}

		m_RenderData.ImagesInFlight[imageIndex] = m_RenderData.InFlightFences[m_RenderData.CurrentFrame];

		VkCommandBuffer& cmd = m_RenderData.CommandBuffers[imageIndex];

		vkResetCommandBuffer(cmd, 0);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		vkBeginCommandBuffer(cmd, &beginInfo);

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		// render to texture

		VkRenderPassBeginInfo textureRPInfo = {};
		textureRPInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		textureRPInfo.renderPass = m_RenderTextureRenderPass;
		textureRPInfo.framebuffer = m_RenderTextureFramebuffer;
		textureRPInfo.renderArea.offset = { 0, 0 };
		textureRPInfo.renderArea.extent = m_CoreData.Swapchain.extent;
		textureRPInfo.clearValueCount = static_cast<u32>(clearValues.size());
		textureRPInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(cmd, &textureRPInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = m_RenderTextureWidth;
		viewport.height = m_RenderTextureHeight;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = { m_RenderTextureWidth, m_RenderTextureHeight };
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsShader.Pipeline);

		for (const auto& obj : objects)
		{
			MVP mvp = {};
			mvp.View = camera.GetViewMatrix();
			mvp.Projection = camera.GetProjectionMatrix();
			mvp.Model = obj->GetTransform().GetModelMatrix();

			vkCmdPushConstants(cmd, m_GraphicsShader.PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MVP), &mvp);

			obj->Bind(cmd);
			obj->Draw(cmd);
		}

		vkCmdEndRenderPass(cmd);

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
			cmd,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		// render to swapchain
		VkRenderPassBeginInfo swapchainRPInfo = {};
		swapchainRPInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		swapchainRPInfo.renderPass = m_RenderData.RenderPass;
		swapchainRPInfo.framebuffer = m_RenderData.Framebuffers[imageIndex];
		swapchainRPInfo.renderArea.offset = { 0, 0 };
		swapchainRPInfo.renderArea.extent = m_CoreData.Swapchain.extent;
		swapchainRPInfo.clearValueCount = static_cast<u32>(clearValues.size());
		swapchainRPInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(cmd, &swapchainRPInfo, VK_SUBPASS_CONTENTS_INLINE);

		viewport.width = static_cast<f32>(m_CoreData.Swapchain.extent.width);
		viewport.height = static_cast<f32>(m_CoreData.Swapchain.extent.height);
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		scissor.extent = m_CoreData.Swapchain.extent;
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_BlitShader.Pipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_BlitShader.PipelineLayout, 0, 1, &m_BlitShader.DescriptorSet, 0, nullptr);
		vkCmdDraw(cmd, 3, 1, 0, 0);

		vkCmdEndRenderPass(cmd);
		vkEndCommandBuffer(cmd);

		std::array waitSemaphores = { m_RenderData.AvailableSemaphores[m_RenderData.CurrentFrame] };
		std::array waitStages = { static_cast<VkPipelineStageFlags>(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT) };

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores.data();
		submitInfo.pWaitDstStageMask = waitStages.data();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_RenderData.CommandBuffers[imageIndex];

		std::array signalSemaphores = { m_RenderData.FinishedSemaphores[imageIndex] };
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
		presentInfo.pImageIndices = &imageIndex;

		result = vkQueuePresentKHR(m_RenderData.PresentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			RecreateSwapchain();
		}
		else if (result != VK_SUCCESS)
		{
			LOG_CRITICAL("Failed to present swapchain image!");
		}
		m_RenderData.CurrentFrame = (m_RenderData.CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
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

		for (auto imageView : m_RenderData.SwapchainImageViews)
		{
			vkDestroyImageView(m_CoreData.Device, imageView, nullptr);
		}

		vkDestroyFramebuffer(m_CoreData.Device, m_RenderTextureFramebuffer, nullptr);

		vkDestroyImageView(m_CoreData.Device, m_RenderTexture.View, nullptr);
		vmaDestroyImage(m_Allocator, m_RenderTexture.Image, m_RenderTexture.Allocation);

		vkDestroyImageView(m_CoreData.Device, m_DepthImage.View, nullptr);
		vmaDestroyImage(m_Allocator, m_DepthImage.Image, m_DepthImage.Allocation);

		m_GraphicsShader.Destroy(m_CoreData.Device);
		vkDestroyRenderPass(m_CoreData.Device, m_RenderData.RenderPass, nullptr);

		vkDestroySampler(m_CoreData.Device, m_RenderTextureSampler, nullptr);

		vkb::destroy_swapchain(m_CoreData.Swapchain);
		vmaDestroyAllocator(m_Allocator);
		vkDestroySurfaceKHR(m_CoreData.Instance, m_CoreData.Surface, nullptr);
		vkb::destroy_device(m_CoreData.Device);
		vkb::destroy_instance(m_CoreData.Instance);
	}
}