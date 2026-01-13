#include "Editor.h"

Editor::Editor()
{
	auto obj = AddObject();
	obj->GetComponent<Core::Transform>()->Position = { 0.0f, 0.0f, -3.0f };
	obj->AddComponent<Core::Mesh>();

	Core::Mesh* mesh = obj->GetComponent<Core::Mesh>();
	*mesh = Core::Application::CreateMeshFromOBJ("Cube.obj");

	glm::vec2 framebufferSize = Core::Application::GetFramebufferSize();

	m_Camera.AspectRatio = static_cast<f32>(framebufferSize.x) / static_cast<f32>(framebufferSize.y);

	glfwSetWindowUserPointer(Core::Application::Get().GetWindow(), this);
	glfwSetCursorPosCallback(Core::Application::Get().GetWindow(), [](GLFWwindow* window, double xpos, double ypos)
		{
			static double lastX = xpos;
			static double lastY = ypos;
			f32 xOffset = xpos - lastX;
			f32 yOffset = ypos - lastY;
			lastX = xpos;
			lastY = ypos;
			Editor* editor = reinterpret_cast<Editor*>(glfwGetWindowUserPointer(window));
			editor->m_Camera.Rotate(xOffset, yOffset, Core::Application::Get().GetDeltaTime());
		});
}

void Editor::OnEvent()
{
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
	Core::Application& app = Core::Application::Get();

	if (glfwGetKey(app.GetWindow(), GLFW_KEY_W) == GLFW_PRESS)
		m_Camera.Move(m_Camera.Front, app.GetDeltaTime());
	if (glfwGetKey(app.GetWindow(), GLFW_KEY_S) == GLFW_PRESS)
		m_Camera.Move(-m_Camera.Front, app.GetDeltaTime());
	if (glfwGetKey(app.GetWindow(), GLFW_KEY_A) == GLFW_PRESS)
		m_Camera.Move(-m_Camera.Right, app.GetDeltaTime());
	if (glfwGetKey(app.GetWindow(), GLFW_KEY_D) == GLFW_PRESS)
		m_Camera.Move(m_Camera.Right, app.GetDeltaTime());

	if (glfwGetKey(app.GetWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetInputMode(app.GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

std::shared_ptr<Core::Object> Editor::AddObject()
{
	return m_Objects.emplace_back(std::make_shared<Core::Object>(m_ECS));
}

void Editor::CreateMesh(const std::shared_ptr<Core::Object>& obj, const std::vector<Core::Vertex>& vertices, const std::vector<u32>& indices)
{
	obj->AddComponent<Core::Mesh>(Core::Application::Get().CreateMeshBuffers(vertices, indices));
}