#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Types.h"

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

		f32 Fov = 45.0f;
		f32 AspectRatio = 16.0f / 9.0f;
		f32 NearPlane = 0.1f;
		f32 FarPlane = 100.0f;

		glm::mat4 GetViewMatrix() const;
		glm::mat4 GetProjectionMatrix() const;

		void Move(const glm::vec3& direction, f32 deltaTime);
		void Rotate(f32 xOffset, f32 yOffset, f32 deltaTime);

	private:
		void RecalculateVectors();

		f32 m_Pitch = 0.0f;
		f32 m_Yaw = -90.0f;
		f32 m_Sensitivity = 100.0f;
		f32 m_Speed = 5.0f;

		static inline const glm::vec3 s_WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	};
}