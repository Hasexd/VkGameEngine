#pragma once

#include <vector>
#include <array>

#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>

#include "Types.h"
#include "VkTypes.h"
#include "Log.h"
#include "Component.h"

namespace Core
{
	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Color;
	};

	class Mesh : public Component
	{
	public:
		Mesh() = default;
		Mesh(const MeshBuffers& meshBuffers);
		~Mesh() = default;

		void Destroy(VmaAllocator allocator);

		void Bind(VkCommandBuffer commandBuffer) const;
		void Draw(VkCommandBuffer commandBuffer) const;

		[[nodiscard]] u32 GetIndexCount() const noexcept { return m_IndexCount; }
		[[nodiscard]] u32 GetVertexCount() const noexcept { return m_VertexCount; }

	private:
		Buffer m_VertexBuffer;
		Buffer m_IndexBuffer;
		u32 m_VertexCount = 0;
		u32 m_IndexCount = 0;
	};
}
