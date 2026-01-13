#include "Editor.h"

Editor::Editor()
{
	auto obj = AddObject<Cube>();
	obj->GetComponent<Core::Transform>()->Position = { 0.0f, 0.0f, -5.0f };

	glm::vec2 framebufferSize = Core::Application::GetWindow().GetFramebufferSize();
	m_Camera.AspectRatio = static_cast<f32>(framebufferSize.x) / static_cast<f32>(framebufferSize.y);

	Core::Application::Get().SetCursorState(GLFW_CURSOR_DISABLED);
	Core::Application::Get().SetBackgroundColor({0.0f, 0.0f, 0.0f, 1.0f});
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
	if (event.GetMouseButton() == GLFW_MOUSE_BUTTON_LEFT && Core::Application::GetCursorState() == GLFW_CURSOR_NORMAL)
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
	return true;
}