#pragma once

#include <glm/glm.hpp>

#include "Types.h"
#include "Object.h"

namespace Core
{

	struct HitResult
	{
		Object* HitObject;
		f32 HitDistance;
		bool Hit;
	};

	struct Ray
	{
		glm::vec3 Origin;
		glm::vec3 Direction;
	};
}