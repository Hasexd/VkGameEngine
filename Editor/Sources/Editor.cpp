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
	auto& app = Core::Application::Get();
	VkPhysicalDeviceProperties props;

	vkGetPhysicalDeviceProperties(app.GetPhysicalDevice(), &props);
	s_MaxLineWidth = props.limits.lineWidthRange[1];

	app.SetCursorState(GLFW_CURSOR_DISABLED);
	app.SetBackgroundColor({0.0f, 0.0f, 0.0f, 1.0f});

	InitImGui();

	app.SetPreFrameRenderFunction([this]() -> void 
		{
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		});

	CreateOutlinePipeline();
	CreateDebugLinePipeline();

	auto obj = AddObject<Cube>("Red cube");
	obj->GetComponent<Core::Transform>()->Position = { 0.0f, 0.0f, 5.0f };
	obj->AddComponent<Core::Material>();

	auto material = obj->GetComponent<Core::Material>();
	material->Color = glm::vec3(1.0f, 0.0f, 0.0f);

	auto obj2 = AddObject<Cube>("Green cube");
	obj2->GetComponent<Core::Transform>()->Position = { -5.0f, 0.0f, 5.0f };
	obj2->AddComponent<Core::Material>();

	material = obj2->GetComponent<Core::Material>();
	material->Color = glm::vec3(0.0f, 1.0f, 0.0f);

	auto obj3 = AddObject<Plane>("Floor");
	obj3->GetComponent<Core::Transform>()->Position = { 0.0f, -2.0f, 0.0f };
	obj3->GetComponent<Core::Transform>()->Scale = { 20.0f, 1.0f, 20.0f };
	obj3->AddComponent<Core::Material>();
	material = obj3->GetComponent<Core::Material>();
	material->Color = glm::vec3(0.5f, 0.5f, 0.5f);

	/*auto porsche = AddObject<Core::Object>("Porsche 911");
	porsche->GetComponent<Core::Transform>()->Position = { 5.0f, 2.0f, 10.0f };
	porsche->AddComponent<Core::Mesh>(Core::Application::CreateMeshFromOBJ("Porsche_911_GT2.obj"));
	porsche->AddComponent<Core::Material>();
	material = porsche->GetComponent<Core::Material>();
	material->Color = glm::vec3(1.0f, 1.0f, 1.0f);*/

	glm::vec2 framebufferSize = app.GetWindow().GetFramebufferSize();
	m_Camera.AspectRatio = static_cast<f32>(framebufferSize.x) / static_cast<f32>(framebufferSize.y);
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
	m_OutlineFillShader.Destroy(Core::Application::Get().GetVulkanDevice());
	m_DebugLineShader.Destroy(Core::Application::Get().GetVulkanDevice());

	m_DebugLines.clear();
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

	UpdateVPData();

	RenderObjects(app);
	RenderSelectedObjectOutline(app);
	RenderDebugLines(app);
}

void Editor::RenderObjects(Core::Application& app)
{
	for (const auto& obj : m_Objects)
	{
		if (obj->HasComponent<Core::Mesh>())
		{
			PushConstants(obj.get());
			obj->Draw(Core::Application::Get().GetCurrentCommandBuffer());
		}
	}
}

void Editor::RenderSelectedObjectOutline(Core::Application& app)
{
	if (!m_SelectedObject || !m_SelectedObject->HasComponent<Core::Mesh>())
		return;

	vkCmdBindPipeline(app.GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_OutlineShader.Pipeline);
	vkCmdBindDescriptorSets(app.GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_OutlineShader.PipelineLayout, 0, 1, &m_OutlineShader.DescriptorSet, 0, nullptr);
	vkCmdSetLineWidth(app.GetCurrentCommandBuffer(), 3.0f);

	PushConstants(m_SelectedObject);
	m_SelectedObject->Draw(app.GetCurrentCommandBuffer());

	vkCmdBindPipeline(app.GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_OutlineFillShader.Pipeline);
	vkCmdBindDescriptorSets(app.GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_OutlineFillShader.PipelineLayout, 0, 1, &m_OutlineFillShader.DescriptorSet, 0, nullptr);

	PushConstants(m_SelectedObject);
	m_SelectedObject->Draw(app.GetCurrentCommandBuffer());
}

void Editor::RenderDebugLines(Core::Application& app)
{
	vkCmdBindPipeline(app.GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_DebugLineShader.Pipeline);
	vkCmdBindDescriptorSets(app.GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_DebugLineShader.PipelineLayout, 0, 1, &m_DebugLineShader.DescriptorSet, 0, nullptr);

	for (const auto& line : m_DebugLines)
	{
		DLPushConstants dlPc = {};
		dlPc.Color = line->GetColor();

		vkCmdPushConstants(app.GetCurrentCommandBuffer(), m_DebugLineShader.PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DLPushConstants), &dlPc);
		vkCmdSetLineWidth(app.GetCurrentCommandBuffer(), line->Thickness);
		line->Draw();
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
	auto& app = Core::Application::Get();
	if (!glfwGetWindowAttrib(app.GetWindow().GetHandle(), GLFW_FOCUSED))
		return;

	if (m_PressedKeys.count(GLFW_KEY_W))
		m_Camera.Move(-m_Camera.Front, deltaTime);
	if (m_PressedKeys.count(GLFW_KEY_S))
		m_Camera.Move(m_Camera.Front, deltaTime);
	if (m_PressedKeys.count(GLFW_KEY_A))
		m_Camera.Move(-m_Camera.Right, deltaTime);
	if (m_PressedKeys.count(GLFW_KEY_D))
		m_Camera.Move(m_Camera.Right, deltaTime);

	for(const auto& obj : m_Objects)
		obj->OnUpdate(deltaTime);

	for (auto& line : m_DebugLines)
	{
		line->Lifetime -= deltaTime;
	}

	vkDeviceWaitIdle(app.GetVulkanDevice());
	m_DebugLines.erase(std::remove_if(m_DebugLines.begin(), m_DebugLines.end(),
		[](const std::unique_ptr<DebugLine>& line) { return line->Lifetime <= 0.0f; }),
		m_DebugLines.end());
}

bool Editor::OnKeyPressed(Core::KeyPressedEvent& event)
{
	m_PressedKeys.insert(event.GetKeyCode());

	if (event.GetKeyCode() == GLFW_KEY_ESCAPE)
		Core::Application::Get().SetCursorState(GLFW_CURSOR_NORMAL);

	if(event.GetKeyCode() == GLFW_KEY_F1 && !event.IsRepeat())
	{
		m_WireframeMode = !m_WireframeMode;
		Core::Application::Get().SetWireframeMode(m_WireframeMode);
	}

	return true;
}

bool Editor::OnKeyReleased(Core::KeyReleasedEvent& event)
{
	m_PressedKeys.erase(event.GetKeyCode());
	return true;
}

bool Editor::OnMouseMoved(Core::MouseMovedEvent& event)
{
	if (Core::Application::Get().GetCursorState() == GLFW_CURSOR_NORMAL)
		return false;

	if (m_LastMouseX == 0.0 && m_LastMouseY == 0.0)
	{
		m_LastMouseX = event.GetX();
		m_LastMouseY = event.GetY();
	}

	f32 xOffset = m_LastMouseX - event.GetX();
	f32 yOffset = m_LastMouseY - event.GetY();
	m_LastMouseX = event.GetX();
	m_LastMouseY = event.GetY();

	m_Camera.Rotate(xOffset, yOffset, Core::Application::GetDeltaTime());

	return true;
}

bool Editor::OnMouseButtonPressed(Core::MouseButtonPressedEvent& event)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return true;

	if (event.GetMouseButton() == GLFW_MOUSE_BUTTON_LEFT && Core::Application::Get().GetCursorState() == GLFW_CURSOR_NORMAL)
	{
		Core::Ray ray = {};
		ray.Origin = m_Camera.Position;
		ray.Origin.z += 0.01f;

		glm::vec2 mousePos = Core::Application::Get().GetWindow().GetMousePos();
		glm::vec2 framebufferSize = Core::Application::Get().GetWindow().GetFramebufferSize();

		glm::vec3 ndc = glm::vec3((2.0f * mousePos.x) / framebufferSize.x - 1.0f, (2.0f * mousePos.y) / framebufferSize.y - 1.0f, 0.0f);
		glm::vec4 rayClip = glm::vec4(ndc.x, ndc.y, 0.0f, 1.0f);
		glm::vec4 rayEye = glm::inverse(m_Camera.GetProjectionMatrix()) * rayClip;
		rayEye = glm::vec4(rayEye.x, rayEye.y, 1.0f, 0.0f);

		glm::vec4 invRayWorld = glm::inverse(m_Camera.GetViewMatrix()) * rayEye;
		glm::vec3 direction(invRayWorld.x, invRayWorld.y, invRayWorld.z);

		ray.Direction = glm::normalize(direction);

		if (PickObject(ray))
		{
			return true;
		}
	}

	if (event.GetMouseButton() == GLFW_MOUSE_BUTTON_LEFT && Core::Application::Get().GetCursorState() == GLFW_CURSOR_NORMAL)
	{
		Core::Application::Get().SetCursorState(GLFW_CURSOR_DISABLED);
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
	m_OutlineFillShader.Destroy(Core::Application::Get().GetVulkanDevice());
	m_DebugLineShader.Destroy(Core::Application::Get().GetVulkanDevice());

	CreateOutlinePipeline();
	CreateDebugLinePipeline();

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

	std::vector<Core::DescriptorBinding> bindings =
	{
		Core::DescriptorBinding(app.GetVPBuffer(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
	};

	std::vector<VkDynamicState> dynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	auto vert = m_ShaderDirectory / "Compiled" / "outline.vert.spv";
	auto frag = m_ShaderDirectory / "Compiled" / "outline.frag.spv";

	VkPipelineDepthStencilStateCreateInfo depthStencilWireframe = {};
	depthStencilWireframe.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilWireframe.depthTestEnable = VK_TRUE;
	depthStencilWireframe.depthWriteEnable = VK_FALSE;
	depthStencilWireframe.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilWireframe.depthBoundsTestEnable = VK_FALSE;
	depthStencilWireframe.stencilTestEnable = VK_FALSE;

	m_OutlineShader = app.CreateShader(
		app.GetRenderTextureRenderPass(),
		bindings,
		{ objPushConstantRange },
		&bindingDescription,
		attributeDescriptions,
		&viewport,
		&scissor,
		&depthStencilWireframe,
		dynamicStates,
		VK_CULL_MODE_FRONT_BIT,
		VK_POLYGON_MODE_LINE,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		vert,
		frag
	);

	app.UpdateDescriptorSets(m_OutlineShader);

	VkPipelineDepthStencilStateCreateInfo depthStencilFill = {};
	depthStencilFill.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilFill.depthTestEnable = VK_TRUE;
	depthStencilFill.depthWriteEnable = VK_TRUE;
	depthStencilFill.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilFill.depthBoundsTestEnable = VK_FALSE;
	depthStencilFill.stencilTestEnable = VK_FALSE;

	m_OutlineFillShader = app.CreateShader(
		app.GetRenderTextureRenderPass(),
		bindings,
		{ objPushConstantRange },
		&bindingDescription,
		attributeDescriptions,
		&viewport,
		&scissor,
		&depthStencilFill,
		dynamicStates,
		VK_CULL_MODE_BACK_BIT,
		VK_POLYGON_MODE_FILL,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		vert,
		frag
	);

	app.UpdateDescriptorSets(m_OutlineFillShader);
}

void Editor::CreateDebugLinePipeline()
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

	VkPushConstantRange debugLinePushConstant = {};
	debugLinePushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	debugLinePushConstant.offset = 0;
	debugLinePushConstant.size = sizeof(DLPushConstants);

	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(glm::vec3);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	attributeDescriptions.resize(1);

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = 0;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	std::vector<VkDynamicState> dynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	std::vector<Core::DescriptorBinding> bindings =
	{
		Core::DescriptorBinding(app.GetVPBuffer(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
	};

	auto vert = m_ShaderDirectory / "Compiled" / "debug_line.vert.spv";
	auto frag = m_ShaderDirectory / "Compiled" / "debug_line.frag.spv";

	m_DebugLineShader = app.CreateShader(app.GetRenderTextureRenderPass(), bindings, { debugLinePushConstant },
		&bindingDescription, attributeDescriptions, &viewport, &scissor, &depthStencil, dynamicStates, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, vert, frag);

	app.UpdateDescriptorSets(m_DebugLineShader);
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
		auto transform = m_SelectedObject->GetComponent<Core::Transform>();
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
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), Core::Application::Get().GetCurrentCommandBuffer());
}

void Editor::DrawDebugLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, f32 lifetime, f32 thickness)
{
	if (thickness < 1.0f || thickness > s_MaxLineWidth)
	{
		LOG_WARN("Debug line thickness: {} is out of the acceptable bounds [{} - {}], the value will be clamped.", thickness, 1.0f, s_MaxLineWidth);
		thickness = glm::clamp(thickness, 1.0f, s_MaxLineWidth);
	}

	m_DebugLines.push_back(std::make_unique<DebugLine>(start, end, color, lifetime, thickness));
}

bool Editor::PickObject(const Core::Ray& ray)
{
	f32 closestDistance = std::numeric_limits<f32>::max();
	Core::Object* closestObject = nullptr;

	for (const auto& obj : m_Objects)
	{
		if (!obj->HasComponent<Core::Mesh>())
			continue;

		auto transform = obj->GetComponent<Core::Transform>();
		glm::mat4 invTransform = glm::inverse(transform->GetModelMatrix());
		glm::vec3 localOrigin = glm::vec3(invTransform * glm::vec4(ray.Origin, 1.0f));
		glm::vec3 localDirection = glm::normalize(glm::vec3(invTransform * glm::vec4(ray.Direction, 0.0f)));

		auto mesh = obj->GetComponent<Core::Mesh>();
		const auto& vertices = mesh->GetVertices();
		const auto& indices = mesh->GetIndices();

		for (usize i = 0; i < indices.size(); i += 3)
		{
			if (indices[i] >= vertices.size() ||
				indices[i + 1] >= vertices.size() ||
				indices[i + 2] >= vertices.size())
			{
				continue;
			}

			glm::vec3 v0 = vertices[indices[i]].Position;
			glm::vec3 v1 = vertices[indices[i + 1]].Position;
			glm::vec3 v2 = vertices[indices[i + 2]].Position;

			Core::Ray localRay = {};
			localRay.Origin = localOrigin;
			localRay.Direction = localDirection;

			f32 distance;
			if (RayTriangleIntersection(localRay, v0, v1, v2, distance))
			{
				glm::vec3 localIntersection = localOrigin + localDirection * distance;
				glm::vec3 worldIntersection = glm::vec3(transform->GetModelMatrix() * glm::vec4(localIntersection, 1.0f));
				f32 worldDistance = glm::length(worldIntersection - ray.Origin);

				if (worldDistance < closestDistance)
				{
					closestDistance = worldDistance;
					closestObject = obj.get();
				}
			}
		}
	}

	if (closestObject != nullptr)
	{
		m_SelectedObject = closestObject;
		return true;
	}

	return false;
}

bool Editor::RayTriangleIntersection(const Core::Ray& ray, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, f32& outDistance)
{
	// https://en.wikipedia.org/wiki/Möller–Trumbore_intersection_algorithm

	constexpr f32 epsilon = std::numeric_limits<f32>::epsilon();

	glm::vec3 edge1 = v1 - v0;
	glm::vec3 edge2 = v2 - v0;
	glm::vec3 rayCrossEdge2 = glm::cross(ray.Direction, edge2);
	f32 det = glm::dot(edge1, rayCrossEdge2);

	if (det > -epsilon && det < epsilon)
		return false;

	f32 invDet = 1.0f / det;
	glm::vec3 s = ray.Origin - v0;
	f32 u = invDet * glm::dot(s, rayCrossEdge2);

	if ((u < 0 && std::abs(u) > epsilon) || (u > 1 && std::abs(u - 1) > epsilon))
		return false;

	glm::vec3 sCrossEdge1 = glm::cross(s, edge1);
	f32 v = invDet * glm::dot(ray.Direction, sCrossEdge1);

	if ((v < 0 && std::abs(v) > epsilon) || (u + v > 1 && std::abs(u + v - 1) > epsilon))
		return false;

	f32 t = invDet * glm::dot(edge2, sCrossEdge1);

	if (t > epsilon)
	{
		outDistance = t;
		return true;
	}

	return false;
}