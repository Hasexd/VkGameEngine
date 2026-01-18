#include "Gizmo.h"


Gizmo::Gizmo(GizmoType type, GizmoAxis axis)
	: m_Type(type), m_Axis(axis)
{
	//length of the cylinder in blender is 0.8

	auto& app = Core::Application::Get();

	if (type == GizmoType::Translate)
		m_Mesh = app.CreateMeshFromOBJ("TranslateGizmo.obj");
	else if (type == GizmoType::Rotate)
		m_Mesh = app.CreateMeshFromOBJ("RotateGizmo.obj");
	else if (type == GizmoType::Scale)
		m_Mesh = app.CreateMeshFromOBJ("ScaleGizmo.obj");

	if (axis == GizmoAxis::X)
	{
		m_Transform.Rotation = glm::radians(glm::vec3(0.0f, 0.0f, -90.0f));
		m_Color = glm::vec3(1.0f, 0.0f, 0.0f);
		m_LocalOffset = glm::vec3(1.0f, 0.0f, 0.0f);
	}
	else if (axis == GizmoAxis::Y)
	{
		m_Transform.Rotation = glm::radians(glm::vec3(0.0f, 0.0f, 0.0f));
		m_Color = glm::vec3(0.0f, 1.0f, 0.0f);
		m_LocalOffset = glm::vec3(0.0f, 1.0f, 0.0f);
	}
	else if (axis == GizmoAxis::Z)
	{
		m_Transform.Rotation = glm::radians(glm::vec3(90.0f, 0.0f, 0.0f));
		m_Color = glm::vec3(0.0f, 0.0f, 1.0f);
		m_LocalOffset = glm::vec3(0.0f, 0.0f, 1.0f);
	}
}

void Gizmo::GetRotationMatrix(glm::mat4& modelMatrix) const noexcept
{
	modelMatrix = glm::rotate(modelMatrix, m_Transform.Rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
	modelMatrix = glm::rotate(modelMatrix, m_Transform.Rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
	modelMatrix = glm::rotate(modelMatrix, m_Transform.Rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
}

Gizmo::~Gizmo()
{
	m_Mesh.Destroy(Core::Application::Get().GetVmaAllocator());
}

void Gizmo::Draw(VkCommandBuffer commandBuffer) const
{
	m_Mesh.Draw(commandBuffer);
}