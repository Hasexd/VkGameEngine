#pragma once

#include <glm/glm.hpp>
#include <filesystem>

#include "Component.h"

namespace Core
{
	class Material : public Asset
	{
	public:
		void LoadFromFile(const std::filesystem::path& path);
		[[nodiscard]] const glm::vec3& GetColor() const noexcept { return m_Color; }
		void SetColor (const glm::vec3& color) noexcept { m_Color = color; }
	private:
		glm::vec3 m_Color = glm::vec3(1.0f);
	};
}