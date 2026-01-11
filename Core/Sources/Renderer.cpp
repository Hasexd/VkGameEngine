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
}

namespace Core
{

	void Renderer::Init(GLFWwindow* window)
	{
		m_Window = window;
		InitCoreData();
		CreateSwapchain();
		GetQueues();
		CreateRP();
		CreateGP();
		CreateFramebuffers();
		CreateCommandPool();
		CreateCommandBuffers();
		CreateSyncObjects();
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

	void Renderer::CreateRP()
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = m_CoreData.Swapchain.image_format;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo rpCreateInfo = {};
		rpCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rpCreateInfo.attachmentCount = 1;
		rpCreateInfo.pAttachments = &colorAttachment;
		rpCreateInfo.subpassCount = 1;
		rpCreateInfo.pSubpasses = &subpass;
		rpCreateInfo.dependencyCount = 1;
		rpCreateInfo.pDependencies = &dependency;

		vkCreateRenderPass(m_CoreData.Device, &rpCreateInfo, nullptr, &m_RenderData.RenderPass);

		ASSERT(m_RenderData.RenderPass);
	}

	void Renderer::CreateGP()
	{
		std::filesystem::path pathToCompiled = std::filesystem::path(PATH_TO_SHADERS) / "Compiled";
		auto vertCode = ReadFile(pathToCompiled / "triangle.vert.spv");
		auto fragCode = ReadFile(pathToCompiled / "triangle.frag.spv");

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

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertStageInfo, fragStageInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;

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
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
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

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pushConstantRangeCount = 0;

		vkCreatePipelineLayout(m_CoreData.Device, &pipelineLayoutInfo, nullptr, &m_RenderData.PipelineLayout);
		ASSERT(m_RenderData.PipelineLayout);

		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

		VkPipelineDynamicStateCreateInfo dynamicInfo = {};
		dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicInfo.pDynamicStates = dynamicStates.data();

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicInfo;
		pipelineInfo.layout = m_RenderData.PipelineLayout;
		pipelineInfo.renderPass = m_RenderData.RenderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		vkCreateGraphicsPipelines(m_CoreData.Device, nullptr, 1, &pipelineInfo, nullptr, &m_RenderData.GraphicsPipeline);
		ASSERT(m_RenderData.GraphicsPipeline);

		vkDestroyShaderModule(m_CoreData.Device, fragModule, nullptr);
		vkDestroyShaderModule(m_CoreData.Device, vertModule, nullptr);
	}

	void Renderer::CreateFramebuffers()
	{
		m_RenderData.SwapchainImages = m_CoreData.Swapchain.get_images().value();
		m_RenderData.SwapchainImageViews = m_CoreData.Swapchain.get_image_views().value();

		m_RenderData.Framebuffers.resize(m_RenderData.SwapchainImageViews.size());

		for (usize i = 0; i < m_RenderData.SwapchainImageViews.size(); i++)
		{
			VkImageView attachments[] = { m_RenderData.SwapchainImageViews[i] };

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderData.RenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
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
		
		for (usize i = 0; i < m_RenderData.CommandBuffers.size(); i++)
		{
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			vkBeginCommandBuffer(m_RenderData.CommandBuffers[i], &beginInfo);

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = m_RenderData.RenderPass;
			renderPassInfo.framebuffer = m_RenderData.Framebuffers[i];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = m_CoreData.Swapchain.extent;

			VkClearValue clearColor{ { { 0.0f, 0.0f, 0.0f, 1.0f } } };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

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

			vkCmdSetViewport(m_RenderData.CommandBuffers[i], 0, 1, &viewport);
			vkCmdSetScissor(m_RenderData.CommandBuffers[i], 0, 1, &scissor);

			vkCmdBeginRenderPass(m_RenderData.CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(m_RenderData.CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_RenderData.GraphicsPipeline);
			vkCmdDraw(m_RenderData.CommandBuffers[i], 3, 1, 0, 0);
			vkCmdEndRenderPass(m_RenderData.CommandBuffers[i]);

			vkEndCommandBuffer(m_RenderData.CommandBuffers[i]);
		}
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

	void Renderer::RecreateSwapchain()
	{
		vkDeviceWaitIdle(m_CoreData.Device);
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
		CreateFramebuffers();
		CreateCommandPool();
		CreateCommandBuffers();
	}

	void Renderer::DrawFrame()
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

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_RenderData.AvailableSemaphores[m_RenderData.CurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_RenderData.CommandBuffers[imageIndex];

		VkSemaphore signalSemaphores[] = { m_RenderData.FinishedSemaphores[imageIndex] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(m_CoreData.Device, 1, &m_RenderData.InFlightFences[m_RenderData.CurrentFrame]);

		vkQueueSubmit(m_RenderData.GraphicsQueue, 1, &submitInfo, m_RenderData.InFlightFences[m_RenderData.CurrentFrame]);

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapchains[] = { m_CoreData.Swapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
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
}