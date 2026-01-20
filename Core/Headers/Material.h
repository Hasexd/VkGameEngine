#pragma once

#include <filesystem>
#include <fstream>
#include <string>

#include <glm/glm.hpp>

#include "Component.h"
#include "Log.h"

namespace Core
{
	class Material : public Asset
	{
	public:
		void LoadFromFile(const std::filesystem::path& path);

		glm::vec3 Ambient;
		glm::vec3 Diffuse;
		glm::vec3 Specular;
		f32 Shininess;
	};
}