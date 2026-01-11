#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Core
{
	struct Camera
	{
		glm::vec3 Position = glm::vec3(0.0f, 0.0f, 3.0f);
		glm::vec3 Front = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);

		float Fov = 45.0f;
		float AspectRatio = 16.0f / 9.0f;
		float NearPlane = 0.1f;
		float FarPlane = 100.0f;

		glm::mat4 GetViewMatrix() const;
		glm::mat4 GetProjectionMatrix() const;
	};
}