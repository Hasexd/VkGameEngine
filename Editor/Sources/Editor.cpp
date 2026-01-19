#include "Editor.h"

namespace
{
	void CheckVkResult(VkResult err)
	{
		if (err == VK_SUCCESS)
			return;

		LOG_ERROR("Vulkan Error: VkResult = {}", static_cast<u32>(err));
	}

	void FlushToZero(glm::vec3& vector)
	{
		if (std::abs(vector.x) < 0.02f) vector.x = 0.f;
		if (std::abs(vector.y) < 0.02f) vector.y = 0.f;
		if (std::abs(vector.z) < 0.02f) vector.z = 0.f;
	}
}

Editor::Editor()
{
	auto& app = Core::Application::Get();
	s_MaxLineWidth = app.GetPhysicalDeviceLimits().lineWidthRange[1];

	app.SetCursorState(GLFW_CURSOR_DISABLED);
	app.SetBackgroundColor({0.0f, 0.0f, 0.0f, 1.0f});

	InitImGui();
	InitGizmos();

	app.SetPreFrameRenderFunction([this]() -> void 
		{
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		});

	CreateOutlinePipeline();
	CreateDebugLinePipeline();
	CreateGizmoPipeline();

	auto obj = AddObject<Cube>("Cube 1");
	obj->GetComponent<Core::Transform>()->Position = { 0.0f, 0.0f, 5.0f };
	obj->AddComponent<Core::Material>();

	auto material = obj->GetComponent<Core::Material>();
	material->Color = glm::vec3(0.3f, 0.2f, 0.6f);

	auto obj2 = AddObject<Cube>("Cube 2");
	obj2->GetComponent<Core::Transform>()->Position = { -5.0f, 0.0f, 5.0f };
	obj2->AddComponent<Core::Material>();

	material = obj2->GetComponent<Core::Material>();
	material->Color = glm::vec3(0.4f, 0.7f, 0.2f);

	auto obj3 = AddObject<Plane>("Floor");
	obj3->GetComponent<Core::Transform>()->Position = { 0.0f, -2.0f, 0.0f };
	obj3->GetComponent<Core::Transform>()->Scale = { 20.0f, 1.0f, 20.0f };
	obj3->AddComponent<Core::Material>();
	material = obj3->GetComponent<Core::Material>();
	material->Color = glm::vec3(0.5f, 0.5f, 0.5f);

	// uncomment for porsche
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
	auto& app = Core::Application::Get();
	for (auto& obj : m_Objects)
	{
		if(obj->HasComponent<Core::Mesh>())
		{
			auto mesh = obj->GetComponent<Core::Mesh>();

			vmaDestroyBuffer(
				app.GetVmaAllocator(),
				mesh->GetVertexBuffer().Buffer,
				mesh->GetVertexBuffer().Allocation);

			vmaDestroyBuffer(
				app.GetVmaAllocator(),
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

	vkDestroyDescriptorPool(app.GetVulkanDevice(), m_ImGuiDescriptorPool, nullptr);
	m_OutlineShader.Destroy(app.GetVulkanDevice());
	m_OutlineFillShader.Destroy(app.GetVulkanDevice());
	m_DebugLineShader.Destroy(app.GetVulkanDevice());
	m_GizmoShader.Destroy(app.GetVulkanDevice());

	m_DebugLines.clear();
	m_Gizmos.clear();
}

void Editor::InitGizmos()
{
	m_Gizmos.emplace_back(std::make_unique<Gizmo>(m_ECS, GizmoType::Translate, GizmoAxis::X));
	m_Gizmos.emplace_back(std::make_unique<Gizmo>(m_ECS, GizmoType::Translate, GizmoAxis::Y));
	m_Gizmos.emplace_back(std::make_unique<Gizmo>(m_ECS, GizmoType::Translate, GizmoAxis::Z));

	
	m_Gizmos.emplace_back(std::make_unique<Gizmo>(m_ECS, GizmoType::Rotate, GizmoAxis::X));
	m_Gizmos.emplace_back(std::make_unique<Gizmo>(m_ECS, GizmoType::Rotate, GizmoAxis::Y));
	m_Gizmos.emplace_back(std::make_unique<Gizmo>(m_ECS, GizmoType::Rotate, GizmoAxis::Z));

	m_Gizmos.emplace_back(std::make_unique<Gizmo>(m_ECS, GizmoType::Scale, GizmoAxis::X));
	m_Gizmos.emplace_back(std::make_unique<Gizmo>(m_ECS, GizmoType::Scale, GizmoAxis::Y));
	m_Gizmos.emplace_back(std::make_unique<Gizmo>(m_ECS, GizmoType::Scale, GizmoAxis::Z));
}

void Editor::OnEvent(Core::Event& event)
{
	Core::EventDispatcher dispatcher(event);

	dispatcher.Dispatch<Core::MouseMovedEvent>([this](Core::MouseMovedEvent& e) { return OnMouseMoved(e); });
	dispatcher.Dispatch<Core::MouseButtonPressedEvent>([this](Core::MouseButtonPressedEvent& e) { return OnMouseButtonPressed(e); });
	dispatcher.Dispatch<Core::MouseButtonReleasedEvent>([this](Core::MouseButtonReleasedEvent& e) { return OnMouseButtonReleased(e); });
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
	RenderGizmos(app);
	RenderDebugLines(app);
}

void Editor::RenderObjects(Core::Application& app)
{
	for (const auto& obj : m_Objects)
	{
		if (obj->HasComponent<Core::Mesh>() && obj->IsVisible())
		{
			PushConstants(obj.get());
			obj->Draw(Core::Application::Get().GetCurrentCommandBuffer());
		}
	}
}

void Editor::RenderGizmos(Core::Application& app)
{
	if (!m_SelectedObject)
		return;

	vkCmdBindPipeline(app.GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_GizmoShader.Pipeline);
	vkCmdBindDescriptorSets(app.GetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_GizmoShader.PipelineLayout, 0, 1, &m_GizmoShader.DescriptorSet, 0, nullptr);

	u32 start = m_ActiveGizmoType == GizmoType::Translate ? 0 :
		m_ActiveGizmoType == GizmoType::Rotate ? 3 : 6;
	u32 end = start + 3;

	const glm::vec3& objPosition = m_SelectedObject->GetComponent<Core::Transform>()->Position;
	const glm::vec3& cameraPos = m_Camera.Position;

	f32 distance = glm::length(cameraPos - objPosition);
	f32 scaleFactor = distance * 0.075f;

	GizmoPushConstants pc = {};

	for (u32 i = start; i < end; i++)
	{
		m_Gizmos[i]->SetPosition(objPosition + m_Gizmos[i]->GetLocalOffset() * scaleFactor);
		m_Gizmos[i]->SetScale(glm::vec3(scaleFactor));

		if (m_ActiveGizmo == m_Gizmos[i].get())
		{
			glm::vec3 baseColor = m_Gizmos[i]->GetColor();
			pc.Color = baseColor + glm::vec3(0.3f);
			pc.Color = glm::min(baseColor + glm::vec3(0.3f), glm::vec3(1.0f));
		}
		else
		{
			pc.Color = m_Gizmos[i]->GetColor();
		}

		pc.Model = m_Gizmos[i]->GetModelMatrix();

		vkCmdPushConstants(app.GetCurrentCommandBuffer(), m_GizmoShader.PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GizmoPushConstants), &pc);
		m_Gizmos[i]->Draw(app.GetCurrentCommandBuffer());
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
		line->Draw(app.GetCurrentCommandBuffer());
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

	if (!m_PressedKeys.count(GLFW_KEY_LEFT_CONTROL) && m_PressedKeys.count(GLFW_KEY_W))
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

	if (event.GetKeyCode() == GLFW_KEY_W && m_PressedKeys.count(GLFW_KEY_LEFT_CONTROL))
	{
		m_ActiveGizmoType = GizmoType::Translate;
		return true;
	}

	if (event.GetKeyCode() == GLFW_KEY_E && m_PressedKeys.count(GLFW_KEY_LEFT_CONTROL))
	{
		m_ActiveGizmoType = GizmoType::Rotate;
		return true;
	}

	if (event.GetKeyCode() == GLFW_KEY_R && m_PressedKeys.count(GLFW_KEY_LEFT_CONTROL))
	{
		m_ActiveGizmoType = GizmoType::Scale;
		return true;
	}

	if (event.GetKeyCode() == GLFW_KEY_ESCAPE)
	{
		Core::Application::Get().SetCursorState(GLFW_CURSOR_NORMAL);
		return true;
	}

	if(event.GetKeyCode() == GLFW_KEY_F1 && !event.IsRepeat())
	{
		m_WireframeMode = !m_WireframeMode;
		Core::Application::Get().SetWireframeMode(m_WireframeMode);

		return true;
	}

	return false;
}

bool Editor::OnKeyReleased(Core::KeyReleasedEvent& event)
{
	m_PressedKeys.erase(event.GetKeyCode());
	return true;
}

bool Editor::OnMouseMoved(Core::MouseMovedEvent& event)
{
	if (m_ActiveGizmo)
	{
		GizmoType type = m_ActiveGizmo->GetType();
		GizmoAxis axis = m_ActiveGizmo->GetAxis();

		if (type == GizmoType::Translate)
		{
			glm::vec3 point = m_SelectedObject->GetComponent<Core::Transform>()->Position;

			if (axis == GizmoAxis::X)
			{
				AxisTranslationDragger(glm::vec3(1.0f, 0.0f, 0.0f), point);

			}
			else if (axis == GizmoAxis::Y)
			{
				AxisTranslationDragger(glm::vec3(0.0f, 1.0f, 0.0f), point);
			}
			else if (axis == GizmoAxis::Z)
			{
				AxisTranslationDragger(glm::vec3(0.0f, 0.0f, 1.0f), point);
			}
			m_SelectedObject->GetComponent<Core::Transform>()->Position = point;
		}
		else if (type == GizmoType::Rotate)
		{
			if (axis == GizmoAxis::X)
			{
				AxisRotationDragger(glm::vec3(1.0f, 0.0f, 0.0f));
			}
			else if (axis == GizmoAxis::Y)
			{
				AxisRotationDragger(glm::vec3(0.0f, 1.0f, 0.0f));
			}
			else if (axis == GizmoAxis::Z)
			{
				AxisRotationDragger(glm::vec3(0.0f, 0.0f, 1.0f));
			}
		}
		else if (type == GizmoType::Scale)
		{
			if (axis == GizmoAxis::X)
			{
				AxisScaleDragger(glm::vec3(1.0f, 0.0f, 0.0f));
			}
			else if (axis == GizmoAxis::Y)
			{
				AxisScaleDragger(glm::vec3(0.0f, 1.0f, 0.0f));
			}
			else if (axis == GizmoAxis::Z)
			{
				AxisScaleDragger(glm::vec3(0.0f, 0.0f, 1.0f));
			}
		}

		m_LastMouseX = event.GetX();
		m_LastMouseY = event.GetY();
		return true;
	}

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

void Editor::PlaneTranslationDragger(const glm::vec3& planeNormal, glm::vec3& point)
{
	const glm::vec3 planePoint = m_SelectedObject->GetComponent<Core::Transform>()->Position;
	const Core::Ray ray = GetMouseRay();
	const f32 denom = glm::dot(planeNormal, ray.Direction);

	if (std::abs(denom) == 0.0f)
		return;

	const f32 t = glm::dot(planePoint - ray.Origin, planeNormal) / denom;
	if (t < 0)
		return;

	point = ray.Origin + ray.Direction * t;
}

void Editor::AxisTranslationDragger(const glm::vec3& axis, glm::vec3& point)
{
	const glm::vec3 planeTangent = glm::cross(axis, point - m_Camera.Position);
	const glm::vec3 planeNormal = glm::cross(axis, planeTangent);

	const Core::Ray ray = GetMouseRay();
	const f32 denom = glm::dot(ray.Direction, planeNormal);

	if (std::abs(denom) < 0.0001f)
		return;

	const f32 t = glm::dot(point - ray.Origin, planeNormal) / denom;

	if (t < 0)
		return;

	glm::vec3 planeIntersection = ray.Origin + ray.Direction * t;
	glm::vec3 projectedPoint = point + axis * glm::dot(planeIntersection - point, axis);
	glm::vec3 delta = projectedPoint - m_ClickOffset;

	point += delta;

	m_ClickOffset = projectedPoint;
}

void Editor::AxisRotationDragger(const glm::vec3& axis)
{
	auto transform = m_SelectedObject->GetComponent<Core::Transform>();

	glm::mat4 rotationMatrix = glm::mat4_cast(transform->GetRotationQuat());
	glm::vec3 worldAxis = glm::normalize(glm::vec3(rotationMatrix * glm::vec4(axis, 0.0f)));

	glm::vec4 plane = glm::vec4(worldAxis, -glm::dot(worldAxis, m_ClickOffset));

	const Core::Ray ray = GetMouseRay();
	const f32 denom = glm::dot(ray.Direction, glm::vec3(plane));

	if (std::abs(denom) < 0.0001f)
		return;

	const f32 t = -(glm::dot(ray.Origin, glm::vec3(plane)) + plane.w) / denom;

	if (t < 0)
		return;

	glm::vec3 currentPoint = ray.Origin + ray.Direction * t;
	glm::vec3 centerOfRotation = transform->Position + worldAxis * glm::dot(m_ClickOffset - transform->Position, worldAxis);

	glm::vec3 arm1 = glm::normalize(m_ClickOffset - centerOfRotation);
	glm::vec3 arm2 = glm::normalize(currentPoint - centerOfRotation);

	f32 d = glm::dot(arm1, arm2);

	if (d > 0.999f)
		return;

	f32 angle = std::acos(glm::clamp(d, -1.0f, 1.0f));

	if (angle < 0.001f)
		return;

	glm::vec3 rotationAxis = glm::normalize(glm::cross(arm1, arm2));

	if (glm::dot(rotationAxis, worldAxis) < 0)
		angle = -angle;

	glm::quat deltaRotation = glm::angleAxis(angle, worldAxis);

	glm::quat currentRotation = transform->GetRotationQuat();
	glm::quat newRotation = deltaRotation * currentRotation;

	transform->Rotation = glm::eulerAngles(newRotation);

	m_ClickOffset = currentPoint;
}

void Editor::AxisScaleDragger(const glm::vec3& axis)
{
	auto transform = m_SelectedObject->GetComponent<Core::Transform>();

	const glm::vec3 planeTangent = glm::cross(axis, transform->Position - m_Camera.Position);
	const glm::vec3 planeNormal = glm::cross(axis, planeTangent);
	const glm::vec3 planePoint = transform->Position;

	const Core::Ray ray = GetMouseRay();

	const f32 denom = glm::dot(ray.Direction, planeNormal);
	
	if (std::abs(denom) == 0)
		return;

	const f32 t = glm::dot(planePoint - ray.Origin, planeNormal) / denom; 

	if (t < 0)
		return;

	const glm::vec3 intersectionPoint = ray.Origin + ray.Direction * t;

	glm::vec3 projectedPoint = transform->Position + axis * glm::dot(intersectionPoint - transform->Position, axis);

	f32 scaleDelta = glm::dot(projectedPoint - m_ClickOffset, axis) * 2.0f;

	if (m_PressedKeys.count(GLFW_KEY_LEFT_CONTROL))
	{
		glm::vec3 newScale = transform->Scale + glm::vec3(scaleDelta);
		transform->Scale = glm::clamp(newScale, glm::vec3(0.01f), glm::vec3(1000.0f));
	}
	else
	{
		glm::vec3 scaleChange = axis * scaleDelta;
		glm::vec3 newScale = transform->Scale + scaleChange;
		transform->Scale = glm::clamp(newScale, glm::vec3(0.01f), glm::vec3(1000.0f));
	}

	m_ClickOffset = projectedPoint;
}

bool Editor::OnMouseButtonPressed(Core::MouseButtonPressedEvent& event)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return true;

	if (event.GetMouseButton() == GLFW_MOUSE_BUTTON_LEFT && Core::Application::Get().GetCursorState() == GLFW_CURSOR_NORMAL)
	{
		if (TestGizmoClick())
			return true;

		if (TestObjectClick())
			return true;

		Core::Application::Get().SetCursorState(GLFW_CURSOR_DISABLED);
		m_LastMouseX = 0.0;
		m_LastMouseY = 0.0;

		return true;
	}

	return false;
}

bool Editor::OnMouseButtonReleased(Core::MouseButtonReleasedEvent& event)
{
	if (event.GetMouseButton() == GLFW_MOUSE_BUTTON_LEFT && m_ActiveGizmo)
	{
		m_ActiveGizmo = nullptr;
		Core::Application::Get().SetCursorState(GLFW_CURSOR_NORMAL);
		return true;
	}

	return false;
}

bool Editor::TestGizmoClick()
{
	if (!m_SelectedObject)
		return false;

	Core::Ray ray = GetMouseRay();
	Core::HitResult hitResult = GizmoRaycast(ray.Origin, ray.Direction, 1e10f);

	if (hitResult.Hit)
	{
		Core::Application::Get().SetCursorState(GLFW_CURSOR_DISABLED);
		m_ActiveGizmo = dynamic_cast<Gizmo*>(hitResult.HitObject);

		auto transform = m_SelectedObject->GetComponent<Core::Transform>();
		glm::vec3 hitPoint = ray.Origin + ray.Direction * hitResult.HitDistance;

		if (m_ActiveGizmo->GetType() == GizmoType::Scale)
		{
			glm::vec3 axis = m_ActiveGizmo->GetAxis() == GizmoAxis::X ? glm::vec3(1, 0, 0) :
				m_ActiveGizmo->GetAxis() == GizmoAxis::Y ? glm::vec3(0, 1, 0) :
				glm::vec3(0, 0, 1);

			m_ClickOffset = transform->Position + axis * glm::dot(hitPoint - transform->Position, axis);
		}
		else if (m_ActiveGizmo->GetType() == GizmoType::Translate)
		{
			glm::vec3 axis = m_ActiveGizmo->GetAxis() == GizmoAxis::X ? glm::vec3(1, 0, 0) :
				m_ActiveGizmo->GetAxis() == GizmoAxis::Y ? glm::vec3(0, 1, 0) :
				glm::vec3(0, 0, 1);

			glm::vec3 planeTangent = glm::cross(axis, transform->Position - m_Camera.Position);
			glm::vec3 planeNormal = glm::cross(axis, planeTangent);

			f32 denom = glm::dot(ray.Direction, planeNormal);
			if (std::abs(denom) > 0.0001f)
			{
				f32 t = glm::dot(transform->Position - ray.Origin, planeNormal) / denom;
				if (t >= 0)
				{
					glm::vec3 planeIntersection = ray.Origin + ray.Direction * t;
					m_ClickOffset = transform->Position + axis * glm::dot(planeIntersection - transform->Position, axis);
				}
				else
				{
					m_ClickOffset = transform->Position;
				}
			}
			else
			{
				m_ClickOffset = transform->Position;
			}
		}
		else
		{
			m_ClickOffset = hitPoint;
		}

		return true;
	}

	return false;
}

bool Editor::TestObjectClick()
{
	Core::Ray ray = GetMouseRay();
	Core::HitResult hitResult = Raycast(ray.Origin, ray.Direction, 1e10f);

	if (hitResult.Hit && hitResult.HitObject != m_SelectedObject)
	{
		m_SelectedObject = hitResult.HitObject;
		return true;
	}

	return false;
}

Core::Ray Editor::GetMouseRay()
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

	return ray;
}

bool Editor::OnWindowResize(Core::WindowResizeEvent& event)
{
	auto& app = Core::Application::Get();
	m_Camera.AspectRatio = static_cast<f32>(event.GetWidth()) / static_cast<f32>(event.GetHeight());
	m_PressedKeys.clear();

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

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = app.GetMSAASamples();

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
		&multisampling,
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
		&multisampling,
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

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = app.GetMSAASamples();

	auto vert = m_ShaderDirectory / "Compiled" / "debug_line.vert.spv";
	auto frag = m_ShaderDirectory / "Compiled" / "debug_line.frag.spv";

	m_DebugLineShader = app.CreateShader(app.GetRenderTextureRenderPass(), bindings, { debugLinePushConstant },
		&bindingDescription, attributeDescriptions, &viewport, &scissor, &depthStencil, dynamicStates, &multisampling,
		VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, vert, frag);

	app.UpdateDescriptorSets(m_DebugLineShader);
}

void Editor::CreateGizmoPipeline()
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

	VkPushConstantRange gizmoPushConstants = {};
	gizmoPushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	gizmoPushConstants.offset = 0;
	gizmoPushConstants.size = sizeof(GizmoPushConstants);

	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Core::Vertex);
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
	};

	std::vector<Core::DescriptorBinding> bindings =
	{
		Core::DescriptorBinding(app.GetVPBuffer(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
	};

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = app.GetMSAASamples();

	auto vert = m_ShaderDirectory / "Compiled" / "gizmo.vert.spv";
	auto frag = m_ShaderDirectory / "Compiled" / "gizmo.frag.spv";

	m_GizmoShader = app.CreateShader(app.GetRenderTextureRenderPass(), bindings, { gizmoPushConstants },
		&bindingDescription, attributeDescriptions, &viewport, &scissor, &depthStencil, dynamicStates, &multisampling,
		VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, vert, frag);

	app.UpdateDescriptorSets(m_GizmoShader);
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

Core::HitResult Editor::GizmoRaycast(const glm::vec3& start, const glm::vec3& direction, f32 maxDistance)
{
	u32 startIdx = m_ActiveGizmoType == GizmoType::Translate ? 0 :
		m_ActiveGizmoType == GizmoType::Rotate ? 3 : 6;
	u32 endIdx = startIdx + 3;

	f32 closestDistance = std::numeric_limits<f32>::max();
	Core::Object* closestObject = nullptr;

	for (u32 i = startIdx; i < endIdx; i++)
	{
		auto& gizmo = m_Gizmos[i];

		if (!gizmo->HasComponent<Core::Mesh>())
			continue;

		auto transform = gizmo->GetComponent<Core::Transform>();

		glm::mat4 invTransform = glm::inverse(transform->GetModelMatrix());
		glm::vec3 localOrigin = glm::vec3(invTransform * glm::vec4(start, 1.0f));
		glm::vec3 localDirection = glm::normalize(glm::vec3(invTransform * glm::vec4(direction, 0.0f)));

		auto mesh = gizmo->GetComponent<Core::Mesh>();
		const auto& vertices = mesh->GetVertices();
		const auto& indices = mesh->GetIndices();

		for (usize j = 0; j < indices.size(); j += 3)
		{
			if (indices[j] >= vertices.size() ||
				indices[j + 1] >= vertices.size() ||
				indices[j + 2] >= vertices.size())
			{
				continue;
			}

			glm::vec3 v0 = vertices[indices[j]].Position;
			glm::vec3 v1 = vertices[indices[j + 1]].Position;
			glm::vec3 v2 = vertices[indices[j + 2]].Position;

			Core::Ray localRay = {};
			localRay.Origin = localOrigin;
			localRay.Direction = localDirection;

			f32 distance;
			if (RayTriangleIntersection(localRay, v0, v1, v2, distance))
			{
				glm::vec3 localIntersection = localOrigin + localDirection * distance;
				glm::vec3 worldIntersection = glm::vec3(transform->GetModelMatrix() * glm::vec4(localIntersection, 1.0f));
				f32 worldDistance = glm::length(worldIntersection - start);

				if (worldDistance < closestDistance && worldDistance <= maxDistance)
				{
					closestDistance = worldDistance;
					closestObject = gizmo.get();
				}
			}
		}
	}

	Core::HitResult result = {};
	result.HitObject = closestObject;
	result.HitDistance = closestDistance;
	result.Hit = (closestObject != nullptr);

	return result;
}

Core::HitResult Editor::Raycast(const glm::vec3& start, const glm::vec3& direction, f32 maxDistance)
{
	return RaycastInternal(start, direction, maxDistance, m_Objects);
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