#pragma once

#include "Transform.h"
#include "Mesh.h"
#include "Application.h"

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

class Gizmo
{
public:
	Gizmo(GizmoType type, GizmoAxis axis);
	~Gizmo();

	[[nodiscard]] GizmoType GetType() const noexcept { return m_Type; }
	[[nodiscard]] GizmoAxis GetAxis() const noexcept { return m_Axis; }
	[[nodiscard]] const glm::vec3& GetColor() const noexcept { return m_Color; }
	[[nodiscard]] const glm::vec3& GetLocalOffset() const noexcept { return m_LocalOffset; }

	void GetRotationMatrix(glm::mat4& modelMatrix) const noexcept;

	void SetPosition(const glm::vec3& position) { m_Transform.Position = position; }
	void SetScale(const glm::vec3& scale) { m_Transform.Scale = scale; }

	void Draw(VkCommandBuffer commandBuffer) const;
private:
	GizmoType m_Type;
	GizmoAxis m_Axis;

	glm::vec3 m_LocalOffset;
	Core::Transform m_Transform;
	Core::Mesh m_Mesh;

	glm::vec3 m_Color;
};