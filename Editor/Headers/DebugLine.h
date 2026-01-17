#pragma once

#include <glm/glm.hpp>

#include "Transform.h"
#include "Mesh.h"
#include "Application.h"

struct DLPushConstants
{
	glm::mat4 Model;
	glm::vec3 Color;
};

class DebugLine
{
public:
	DebugLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, f32 lifetime);
	~DebugLine();
	void Draw() const;

	DebugLine(const DebugLine&) = delete;
	DebugLine& operator=(const DebugLine&) = delete;
	DebugLine(DebugLine&&) = delete;
	DebugLine& operator=(DebugLine&&) = delete;

	glm::mat4 GetModelMatrix() const { return m_Transform.GetModelMatrix(); }
	glm::vec3 GetColor() const { return m_Color; }

public:
	f32 Lifetime = 0.0f;
private:
	void CreateBuffers(const glm::vec3& start, const glm::vec3& end);
private:
	glm::vec3 m_Color;
	Core::Transform m_Transform;
	Core::Buffer m_VertexBuffer;
	static inline Core::Buffer s_IndexBuffer;
	static inline u32 s_Instances = 0;
};