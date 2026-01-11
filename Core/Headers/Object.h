#pragma once

#include "Transform.h"
#include "Mesh.h"

namespace Core
{
	class Object
	{
	public:
		Object() = default;
		~Object() = default;

		void Bind(VkCommandBuffer commandBuffer);
		void Draw(VkCommandBuffer commandBuffer);

		[[nodiscard]] Transform& GetTransform() noexcept { return m_Transform; }
		[[nodiscard]] const Transform& GetTransform() const noexcept { return m_Transform; }
		[[nodiscard]] Mesh& GetMesh() noexcept { return m_Mesh; }
		[[nodiscard]] const Mesh& GetMesh() const noexcept { return m_Mesh; }
	private:
		Transform m_Transform;
		Mesh m_Mesh;
	};
}