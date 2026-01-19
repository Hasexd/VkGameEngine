#include "Transform.h"

namespace Core
{
	glm::mat4 Transform::GetModelMatrix() const
	{
		glm::mat4 model = glm::mat4(1.0f);

		model = glm::translate(model, Position);
		model = glm::rotate(model, Rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, Rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, Rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, Scale);
		
		return model;
	}

	glm::quat Transform::GetRotationQuat() const
	{
		return glm::quat(Rotation);
	}
}