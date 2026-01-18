#pragma once

#include "Transform.h"
#include "Mesh.h"
#include "Application.h"
#include "Object.h"

enum class GizmoType
{
	Translate,
	Rotate,
	Scale
};

enum class GizmoAxis
{
	None,
	X,
	Y,
	Z
};

struct GizmoPushConstants
{
	glm::mat4 Model;
	glm::vec3 Color;
};

class Gizmo : public Core::Object
{
public:
	Gizmo(Core::ECS& ecs, GizmoType type, GizmoAxis axis);
	~Gizmo() override;

	[[nodiscard]] GizmoType GetType() const noexcept { return m_Type; }
	[[nodiscard]] GizmoAxis GetAxis() const noexcept { return m_Axis; }
	[[nodiscard]] const glm::vec3& GetColor() const noexcept { return m_Color; }
	[[nodiscard]] const glm::vec3& GetLocalOffset() const noexcept { return m_LocalOffset; }

	[[nodiscard]] glm::mat4 GetModelMatrix() noexcept;

	void SetPosition(const glm::vec3& position);
	void SetScale(const glm::vec3& scale);
private:
	GizmoType m_Type;
	GizmoAxis m_Axis;

	glm::vec3 m_Color;
	glm::vec3 m_LocalOffset;
};