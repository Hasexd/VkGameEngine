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
		glm::mat4 proj = glm::perspective(glm::radians(Fov), AspectRatio, NearPlane, FarPlane);
		proj[1][1] *= -1;

		return proj;
	}

	void Camera::Move(const glm::vec3& direction, float deltaTime)
	{
		Position += direction * m_Speed * deltaTime;
	}

	void Camera::Rotate(double xOffset, double yOffset, double deltaTime)
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