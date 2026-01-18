#include "Gizmo.h"


Gizmo::Gizmo(Core::ECS& ecs, GizmoType type, GizmoAxis axis)
	: Object(ecs, ""), m_Type(type), m_Axis(axis)
{
	//length of the cylinder in blender is 0.8

	AddComponent<Core::Mesh>();

	auto& app = Core::Application::Get();

	auto mesh = GetComponent<Core::Mesh>();
	auto transform = GetComponent<Core::Transform>();

	if (type == GizmoType::Translate)
		*mesh = app.CreateMeshFromOBJ("TranslateGizmo.obj");
	else if (type == GizmoType::Rotate)
		*mesh = app.CreateMeshFromOBJ("RotateGizmo.obj");
	else if (type == GizmoType::Scale)
		*mesh = app.CreateMeshFromOBJ("ScaleGizmo.obj");

	if (axis == GizmoAxis::X)
	{
		transform->Rotation = glm::radians(glm::vec3(0.0f, 0.0f, -90.0f));
		m_Color = glm::vec3(1.0f, 0.0f, 0.0f);
		m_LocalOffset = glm::vec3(1.0f, 0.0f, 0.0f);
	}
	else if (axis == GizmoAxis::Y)
	{
		transform->Rotation = glm::radians(glm::vec3(0.0f, 0.0f, 0.0f));
		m_Color = glm::vec3(0.0f, 1.0f, 0.0f);
		m_LocalOffset = glm::vec3(0.0f, 1.0f, 0.0f);
	}
	else if (axis == GizmoAxis::Z)
	{
		transform->Rotation = glm::radians(glm::vec3(90.0f, 0.0f, 0.0f));
		m_Color = glm::vec3(0.0f, 0.0f, 1.0f);
		m_LocalOffset = glm::vec3(0.0f, 0.0f, 1.0f);
	}
}

Gizmo::~Gizmo()
{
	GetComponent<Core::Mesh>()->Destroy(Core::Application::Get().GetVmaAllocator());
}

glm::mat4 Gizmo::GetModelMatrix() noexcept
{
	return GetComponent<Core::Transform>()->GetModelMatrix();
}

void Gizmo::SetPosition(const glm::vec3& position)
{
	GetComponent<Core::Transform>()->Position = position;
}

void Gizmo::SetScale(const glm::vec3& scale)
{
	GetComponent<Core::Transform>()->Scale = scale;
}
