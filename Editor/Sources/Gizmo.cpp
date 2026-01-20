#include "Gizmo.h"


Gizmo::Gizmo(Core::ECS& ecs, GizmoType type, GizmoAxis axis)
	: Object(ecs, ""), m_Type(type), m_Axis(axis)
{

	auto& app = Core::Application::Get();

	auto mesh = AddComponent<Core::Mesh>();
	auto transform = GetComponent<Core::Transform>();

	if (type == GizmoType::Translate)
	{
		*mesh = app.CreateMeshFromOBJ("TranslateGizmo.obj");
		SetupTranslationGizmo(transform);

	}
	else if (type == GizmoType::Rotate)
	{
		*mesh = app.CreateMeshFromOBJ("RotateGizmo.obj");
		SetupRotationGizmo(transform);
	}
	else if (type == GizmoType::Scale)
	{
		*mesh = app.CreateMeshFromOBJ("ScaleGizmo.obj");
		SetupScaleGizmo(transform);
	}

	if (m_Axis == GizmoAxis::X)
	{
		m_Color = glm::vec3(1.0f, 0.0f, 0.0f);
	}
	else if (m_Axis == GizmoAxis::Y)
	{
		m_Color = glm::vec3(0.0f, 1.0f, 0.0f);
	}
	else if (m_Axis == GizmoAxis::Z)
	{
		m_Color = glm::vec3(0.0f, 0.0f, 1.0f);
	}
}

void Gizmo::SetupTranslationGizmo(Core::Transform* transform)
{
	if (m_Axis == GizmoAxis::X)
	{
		transform->Rotation = glm::radians(glm::vec3(0.0f, 0.0f, -90.0f));
		m_LocalOffset = glm::vec3(1.0f, 0.0f, 0.0f);
	}
	else if (m_Axis == GizmoAxis::Y)
	{
		transform->Rotation = glm::radians(glm::vec3(0.0f, 0.0f, 0.0f));
		m_LocalOffset = glm::vec3(0.0f, 1.0f, 0.0f);
	}
	else if (m_Axis == GizmoAxis::Z)
	{
		transform->Rotation = glm::radians(glm::vec3(90.0f, 0.0f, 0.0f));
		m_LocalOffset = glm::vec3(0.0f, 0.0f, 1.0f);
	}
}

void Gizmo::SetupRotationGizmo(Core::Transform* transform)
{
	if (m_Axis == GizmoAxis::X)
	{
		transform->Rotation = glm::radians(glm::vec3(45.0f, 0.0f, 90.0f));
	}
	else if (m_Axis == GizmoAxis::Y)
	{
		transform->Rotation = glm::radians(glm::vec3(0.0f, -45.0f, 0.0f));
	}
	else if (m_Axis == GizmoAxis::Z)
	{
		transform->Rotation = glm::radians(glm::vec3(45.0f, 270.0f, 90.0f));
	}
}

void Gizmo::SetupScaleGizmo(Core::Transform* transform)
{
	if (m_Axis == GizmoAxis::X)
	{
		transform->Rotation = glm::radians(glm::vec3(0.0f, 0.0f, -90.0f));
		m_LocalOffset = glm::vec3(0.8f, 0.0f, 0.0f);
	}
	else if (m_Axis == GizmoAxis::Y)
	{
		transform->Rotation = glm::radians(glm::vec3(0.0f, 0.0f, 0.0f));
		m_LocalOffset = glm::vec3(0.0f, 0.8f, 0.0f);
	}
	else if (m_Axis == GizmoAxis::Z)
	{
		transform->Rotation = glm::radians(glm::vec3(90.0f, 0.0f, 0.0f));
		m_LocalOffset = glm::vec3(0.0f, 0.0f, 0.8f);
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
