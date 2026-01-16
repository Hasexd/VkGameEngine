#include "Editor.h"

namespace
{
	void CheckVkResult(VkResult err)
	{
		if (err == VK_SUCCESS)
			return;

		LOG_ERROR("Vulkan Error: VkResult = {}", static_cast<u32>(err));
	}
}

Editor::Editor()
{
	auto obj = AddObject<Cube>("Red cube");
	obj->GetComponent<Core::Transform>()->Position = { 0.0f, 0.0f, -5.0f };
	obj->AddComponent<Core::Material>();
	
	auto material = obj->GetComponent<Core::Material>();
	material->Color = glm::vec3(1.0f, 0.0f, 0.0f);

	auto obj2 = AddObject<Cube>("Green cube");
	obj2->GetComponent<Core::Transform>()->Position = { -5.0f, 0.0f, -5.0f };
	obj2->AddComponent<Core::Material>();

	material = obj2->GetComponent<Core::Material>();
	material->Color = glm::vec3(0.0f, 1.0f, 0.0f);


	glm::vec2 framebufferSize = Core::Application::GetWindow().GetFramebufferSize();
	m_Camera.AspectRatio = static_cast<f32>(framebufferSize.x) / static_cast<f32>(framebufferSize.y);

	Core::Application::Get().SetCursorState(GLFW_CURSOR_DISABLED);
	Core::Application::Get().SetBackgroundColor({0.0f, 0.0f, 0.0f, 1.0f});

	InitImGui();

	Core::Application::Get().SetPreFrameRenderFunction([this]() -> void 
		{
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		});

	CreateOutlinePipeline();
}

Editor::~Editor()
{
	for (auto& obj : m_Objects)
	{
		if(obj->HasComponent<Core::Mesh>())
		{
			Core::Mesh* mesh = obj->GetComponent<Core::Mesh>();

			vmaDestroyBuffer(
				Core::Application::Get().GetVmaAllocator(),
				mesh->GetVertexBuffer().Buffer,
				mesh->GetVertexBuffer().Allocation);

			vmaDestroyBuffer(
				Core::Application::Get().GetVmaAllocator(),
				mesh->GetIndexBuffer().Buffer,
				mesh->GetIndexBuffer().Allocation);
		}
	}

	std::filesystem::path pathToImGuiIni = std::filesystem::path(PATH_TO_EDITOR) / "imgui.ini";
	std::string pathStr = pathToImGuiIni.string();

	ImGui::SaveIniSettingsToDisk(pathStr.c_str());

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	vkDestroyDescriptorPool(Core::Application::Get().GetVulkanDevice(), m_ImGuiDescriptorPool, nullptr);
	m_OutlineShader.Destroy(Core::Application::Get().GetVulkanDevice());
}

void Editor::OnEvent(Core::Event& event)
{
	Core::EventDispatcher dispatcher(event);

	dispatcher.Dispatch<Core::MouseMovedEvent>([this](Core::MouseMovedEvent& e) { return OnMouseMoved(e); });
	dispatcher.Dispatch<Core::MouseButtonPressedEvent>([this](Core::MouseButtonPressedEvent& e) { return OnMouseButtonPressed(e); });
	dispatcher.Dispatch<Core::KeyPressedEvent>([this](Core::KeyPressedEvent& e) { return OnKeyPressed(e); });
	dispatcher.Dispatch<Core::KeyReleasedEvent>([this](Core::KeyReleasedEvent& e) { return OnKeyReleased(e); });
	dispatcher.Dispatch<Core::WindowResizeEvent>([this](Core::WindowResizeEvent& e) { return OnWindowResize(e); });
}

void Editor::OnRender()
{
	auto& app = Core::Application::Get();

	RenderObjects();

	vkCmdBindPipeline(app.GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_OutlineShader.Pipeline);
	vkCmdBindDescriptorSets(app.GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_OutlineShader.PipelineLayout, 0, 1, &m_OutlineShader.DescriptorSet, 0, nullptr);

	if (m_SelectedObject && m_SelectedObject->HasComponent<Core::Mesh>())
	{
		PushConstants(m_SelectedObject);
		m_SelectedObject->Draw(app.GetCurrentCommandBuffer());
	}
}

void Editor::RenderObjects()
{
	UpdateVPData();

	for (const auto& obj : m_Objects)
	{
		if (obj->HasComponent<Core::Mesh>())
		{
			PushConstants(obj.get());
			obj->Draw(Core::Application::Get().GetCurrentCommandBuffer());
		}
	}
}

void Editor::UpdateVPData()
{
	Core::Application& app = Core::Application::Get();

	Core::VP vpData = {};
	vpData.View = m_Camera.GetViewMatrix();
	vpData.Projection = m_Camera.GetProjectionMatrix();

	void* data;
	vmaMapMemory(app.GetVmaAllocator(), app.GetVPBuffer().Allocation, &data);
	memcpy(data, &vpData, sizeof(Core::VP));
	vmaUnmapMemory(app.GetVmaAllocator(), app.GetVPBuffer().Allocation);
}

void Editor::PushConstants(Core::Object* obj)
{
	Core::Application& app = Core::Application::Get();
	Core::ObjPushConstants objPC = {};

	Core::MaterialUBO matUBO;
	if (obj->HasComponent<Core::Material>())
	{
		matUBO.Color = obj->GetComponent<Core::Material>()->Color;
	}
	
	objPC.Model = obj->GetComponent<Core::Transform>()->GetModelMatrix();
	objPC.Material = matUBO;

	vkCmdPushConstants(app.GetCurrentCommandBuffer(), app.GetGraphicsPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Core::ObjPushConstants), &objPC);
}

void Editor::OnSwapchainRender()
{
	RenderImGui();
}

void Editor::OnUpdate(f32 deltaTime)
{
	if (!glfwGetWindowAttrib(Core::Application::GetWindow().GetHandle(), GLFW_FOCUSED))
		return;

	if (m_PressedKeys.count(GLFW_KEY_W))
		m_Camera.Move(m_Camera.Front, deltaTime);
	if (m_PressedKeys.count(GLFW_KEY_S))
		m_Camera.Move(-m_Camera.Front, deltaTime);
	if (m_PressedKeys.count(GLFW_KEY_A))
		m_Camera.Move(-m_Camera.Right, deltaTime);
	if (m_PressedKeys.count(GLFW_KEY_D))
		m_Camera.Move(m_Camera.Right, deltaTime);

	for(const auto& obj : m_Objects)
		obj->OnUpdate(deltaTime);
}

bool Editor::OnKeyPressed(Core::KeyPressedEvent& event)
{
	m_PressedKeys.insert(event.GetKeyCode());

	if (event.GetKeyCode() == GLFW_KEY_ESCAPE)
		Core::Application::SetCursorState(GLFW_CURSOR_NORMAL);

	return true;
}

bool Editor::OnKeyReleased(Core::KeyReleasedEvent& event)
{
	m_PressedKeys.erase(event.GetKeyCode());
	return true;
}

bool Editor::OnMouseMoved(Core::MouseMovedEvent& event)
{
	if (Core::Application::GetCursorState() == GLFW_CURSOR_NORMAL)
		return false;

	if (m_LastMouseX == 0.0 && m_LastMouseY == 0.0)
	{
		m_LastMouseX = event.GetX();
		m_LastMouseY = event.GetY();
	}

	f32 xOffset = event.GetX() - m_LastMouseX;
	f32 yOffset = event.GetY() - m_LastMouseY;
	m_LastMouseX = event.GetX();
	m_LastMouseY = event.GetY();

	m_Camera.Rotate(xOffset, yOffset, Core::Application::GetDeltaTime());

	return true;
}

bool Editor::OnMouseButtonPressed(Core::MouseButtonPressedEvent& event)
{
	ImGuiIO& io = ImGui::GetIO();

	if (event.GetMouseButton() == GLFW_MOUSE_BUTTON_LEFT && Core::Application::GetCursorState() == GLFW_CURSOR_NORMAL && !io.WantCaptureMouse)
	{
		Core::Application::SetCursorState(GLFW_CURSOR_DISABLED);
		m_LastMouseX = 0.0;
		m_LastMouseY = 0.0;
		return true;
	}

	return false;
}

bool Editor::OnWindowResize(Core::WindowResizeEvent& event)
{
	m_Camera.AspectRatio = static_cast<f32>(event.GetWidth()) / static_cast<f32>(event.GetHeight());
	m_PressedKeys.clear();

	vkDeviceWaitIdle(Core::Application::Get().GetVulkanDevice());

	m_OutlineShader.Destroy(Core::Application::Get().GetVulkanDevice());

	CreateOutlinePipeline();

	return true;
}

void Editor::CreateOutlinePipeline()
{
	auto& app = Core::Application::Get();
	VkExtent2D extent = app.GetSwapchain().extent;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<f32>(extent.width);
	viewport.height = static_cast<f32>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = { extent.width, extent.height };

	VkPushConstantRange objPushConstantRange = {};
	objPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	objPushConstantRange.offset = 0;
	objPushConstantRange.size = sizeof(Core::ObjPushConstants);

	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Core::Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	attributeDescriptions.resize(3);

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Core::Vertex, Position);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Core::Vertex, Normal);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Core::Vertex, TextureCoordinate);

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_TRUE;
	depthStencil.front.passOp = VK_STENCIL_OP_KEEP;
	depthStencil.front.compareOp = VK_COMPARE_OP_NOT_EQUAL;
	depthStencil.front.reference = 1;
	depthStencil.front.compareMask = 0xFF;
	depthStencil.front.writeMask = 0xFF;
	depthStencil.back = depthStencil.front;

	std::vector<Core::DescriptorBinding> bindings =
	{
		Core::DescriptorBinding(app.GetVPBuffer(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
	};

	auto vert = m_ShaderDirectory / "Compiled" / "outline.vert.spv";
	auto frag = m_ShaderDirectory / "Compiled" / "outline.frag.spv";

	m_OutlineShader = app.CreateShader(app.GetRenderTextureRenderPass(), bindings, {objPushConstantRange},
		&bindingDescription, attributeDescriptions, &viewport, &scissor, &depthStencil, VK_CULL_MODE_NONE, vert, frag);

	app.UpdateDescriptorSets(m_OutlineShader);
}

void Editor::InitImGui()
{
	VkDescriptorPoolSize poolSizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 10 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10 }
	};

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 10;
	poolInfo.poolSizeCount = 3;
	poolInfo.pPoolSizes = poolSizes;

	vkCreateDescriptorPool(Core::Application::Get().GetVulkanDevice(), &poolInfo, nullptr, &m_ImGuiDescriptorPool);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	std::filesystem::path pathToImGuiIni = std::filesystem::path(PATH_TO_EDITOR) / "imgui.ini";
	std::string pathStr = pathToImGuiIni.string();

	ImGui::LoadIniSettingsFromDisk(pathStr.c_str());

	ImGui::StyleColorsDark();

	float mainScale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(mainScale);
	style.FontScaleDpi = mainScale;
	io.ConfigDpiScaleFonts = true;
	io.ConfigDpiScaleViewports = true;

	ImGui_ImplGlfw_InitForVulkan(Core::Application::GetWindow().GetHandle(), true);

	Core::Application& app = Core::Application::Get();

	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = app.GetVulkanInstance();
	initInfo.PhysicalDevice = app.GetPhysicalDevice();
	initInfo.Device = app.GetVulkanDevice();
	initInfo.QueueFamily = app.GetQueueFamily();
	initInfo.Queue = app.GetGraphicsQueue();
	initInfo.PipelineCache = VK_NULL_HANDLE;
	initInfo.DescriptorPool = m_ImGuiDescriptorPool;
	initInfo.MinImageCount = app.GetSwapchainImageCount();
	initInfo.ImageCount = app.GetSwapchainImageCount();
	initInfo.Allocator = nullptr;
	initInfo.PipelineInfoMain.RenderPass = app.GetRenderPass();
	initInfo.PipelineInfoMain.Subpass = 0;
	initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	initInfo.CheckVkResultFn = CheckVkResult;

	ImGui_ImplVulkan_Init(&initInfo);
}

void Editor::RenderImGui()
{
	ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);


	ImGui::SetNextWindowBgAlpha(1.0f);
	ImGui::Begin("Object information");

	if (m_SelectedObject)
	{
		Core::Transform* transform = m_SelectedObject->GetComponent<Core::Transform>();
		ImGui::InputFloat3("Position", &transform->Position.x);
		ImGui::InputFloat3("Rotation", &transform->Rotation.x);
		ImGui::InputFloat3("Scale", &transform->Scale.x);
	}
	
	ImGui::End();

	ImGui::SetNextWindowBgAlpha(1.0f);
	ImGui::Begin("Scene Hierarchy");

	for (const auto& obj : m_Objects)
	{
		if (ImGui::Selectable(obj->GetName().c_str(), m_SelectedObject == obj.get()))
		{
			m_SelectedObject = obj.get();
		}
	}

	ImGui::End();

	ImGui::SetNextWindowBgAlpha(1.0f);
	ImGui::Begin("Assets");

	ImGui::End();

	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), Core::Application::GetCurrentCommandBuffer());
}