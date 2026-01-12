#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Core
{
	class Camera
	{
	public:
		Camera();

		glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 Front = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);

		float Fov = 45.0f;
		float AspectRatio = 16.0f / 9.0f;
		float NearPlane = 0.1f;
		float FarPlane = 100.0f;

		glm::mat4 GetViewMatrix() const;
		glm::mat4 GetProjectionMatrix() const;

		void Move(const glm::vec3& direction, float deltaTime);
		void Rotate(double xOffset, double yOffset, double deltaTime);

	private:
		void RecalculateVectors();

		float m_Pitch = 0.0f;
		float m_Yaw = -90.0f;
		float m_Sensitivity = 100.0f;
		float m_Speed = 5.0f;

		static inline const glm::vec3 s_WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	};
}