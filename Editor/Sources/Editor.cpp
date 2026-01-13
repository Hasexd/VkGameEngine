#include "Editor.h"

Editor::Editor()
{
	auto obj = AddObject();
	obj->GetComponent<Core::Transform>()->Position = { 0.0f, 0.0f, -3.0f };
	obj->AddComponent<Core::Mesh>();

	Core::Mesh* mesh = obj->GetComponent<Core::Mesh>();
	*mesh = Core::Application::CreateMeshFromOBJ("Cube.obj");

	glm::vec2 framebufferSize = Core::Application::GetWindow().GetFramebufferSize();

	m_Camera.AspectRatio = static_cast<f32>(framebufferSize.x) / static_cast<f32>(framebufferSize.y);

	Core::Application::Get().SetInputMode(GLFW_CURSOR_DISABLED);
}

void Editor::OnEvent(Core::Event& event)
{
	Core::EventDispatcher dispatcher(event);

	dispatcher.Dispatch<Core::MouseMovedEvent>([this](Core::MouseMovedEvent& e) { return OnMouseMoved(e); });
	dispatcher.Dispatch<Core::KeyPressedEvent>([this](Core::KeyPressedEvent& e) { return OnKeyPressed(e); });
	dispatcher.Dispatch<Core::KeyReleasedEvent>([this](Core::KeyReleasedEvent& e) { return OnKeyReleased(e); });
}

void Editor::OnRender()
{
	Core::Application& app = Core::Application::Get();
	for (const auto& obj : m_Objects)
	{
		if (obj->HasComponent<Core::Mesh>())
		{
			Core::MVP mvp = {};
			mvp.Model = obj->GetComponent<Core::Transform>()->GetModelMatrix();
			mvp.View = m_Camera.GetViewMatrix();
			mvp.Projection = m_Camera.GetProjectionMatrix();

			vkCmdPushConstants(app.GetCurrentCommandBuffer(), app.GetGraphicsPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Core::MVP), &mvp);
			obj->Draw(app.GetCurrentCommandBuffer());
		}
	}
}

void Editor::OnUpdate(f32 deltaTime)
{
	if (m_PressedKeys.count(GLFW_KEY_W))
		m_Camera.Move(m_Camera.Front, deltaTime);
	if (m_PressedKeys.count(GLFW_KEY_S))
		m_Camera.Move(-m_Camera.Front, deltaTime);
	if (m_PressedKeys.count(GLFW_KEY_A))
		m_Camera.Move(-m_Camera.Right, deltaTime);
	if (m_PressedKeys.count(GLFW_KEY_D))
		m_Camera.Move(m_Camera.Right, deltaTime);
}

bool Editor::OnKeyPressed(Core::KeyPressedEvent& event)
{
	m_PressedKeys.insert(event.GetKeyCode());

	if (event.GetKeyCode() == GLFW_KEY_ESCAPE)
		Core::Application::SetInputMode(GLFW_CURSOR_NORMAL);

	return true;
}

bool Editor::OnKeyReleased(Core::KeyReleasedEvent& event)
{
	m_PressedKeys.erase(event.GetKeyCode());
	return true;
}

bool Editor::OnMouseMoved(Core::MouseMovedEvent& event)
{
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

std::shared_ptr<Core::Object> Editor::AddObject()
{
	return m_Objects.emplace_back(std::make_shared<Core::Object>(m_ECS));
}

void Editor::CreateMesh(const std::shared_ptr<Core::Object>& obj, const std::vector<Core::Vertex>& vertices, const std::vector<u32>& indices)
{
	obj->AddComponent<Core::Mesh>(Core::Application::Get().CreateMeshBuffers(vertices, indices));
}