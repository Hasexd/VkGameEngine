#include "Camera.h"

namespace Core
{
	Camera::Camera()
	{
		RecalculateVectors();
	}

	glm::mat4 Camera::GetViewMatrix() const
	{
		return glm::lookAt(Position, Position + Front, Up);
	}

	glm::mat4 Camera::GetProjectionMatrix() const
	{
		glm::mat4 proj = glm::mat4(0.0f);
		proj[0][0] = (1 / AspectRatio) / glm::tan(Fov / 2);
		proj[1][1] = -(1 / glm::tan(Fov / 2));
		proj[2][2] = FarPlane / (FarPlane - NearPlane);
		proj[2][3] = 1.0f;
		proj[3][2] = -(FarPlane * NearPlane) / (FarPlane - NearPlane);

		return proj;
	}

	void Camera::Move(const glm::vec3& direction, f32 deltaTime)
	{
		Position += direction * m_Speed * deltaTime;
	}

	void Camera::Rotate(f32 xOffset, f32 yOffset, f32 deltaTime)
	{
		m_Yaw += xOffset * m_Sensitivity * deltaTime;
		m_Pitch -= yOffset * m_Sensitivity * deltaTime;
		m_Pitch = glm::clamp(m_Pitch, -89.f, 89.f);
		RecalculateVectors();
	}

	void Camera::RecalculateVectors()
	{
		float yawRad = glm::radians(m_Yaw);
		float pitchRad = glm::radians(m_Pitch);

		glm::vec3 newFront = {};
		newFront.x = cos(yawRad) * cos(pitchRad);
		newFront.y = sin(pitchRad);
		newFront.z = sin(yawRad) * cos(pitchRad);
		Front = glm::normalize(newFront);

		Right = glm::normalize(glm::cross(Front, s_WorldUp));
		Up = glm::normalize(glm::cross(Right, Front));
	}
}