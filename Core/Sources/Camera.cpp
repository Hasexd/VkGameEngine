#include "Camera.h"

namespace Core
{
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
}