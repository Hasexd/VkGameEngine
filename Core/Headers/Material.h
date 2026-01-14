#pragma once

#include <glm/glm.hpp>

#include "Component.h"

namespace Core
{
	struct Material : public Component
	{
		glm::vec3 Color = glm::vec3(1.0f);
	};
}