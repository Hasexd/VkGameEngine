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
#include "OBJ-Loader.h"
#include "Application.h"

namespace Core
{
	using Vertex = objl::Vertex;

	class Mesh : public Asset
	{
	public:
		Mesh() = default;
		Mesh(const MeshBuffers& meshBuffers);
		~Mesh();

		void LoadFromFile(const std::filesystem::path& path);

		void Destroy(VmaAllocator allocator);

		void Draw(VkCommandBuffer commandBuffer) const;

		[[nodiscard]] const std::vector<Vertex>& GetVertices() const noexcept { return m_Vertices; }
		[[nodiscard]] const std::vector<u32>& GetIndices() const noexcept { return m_Indices; }
		[[nodiscard]] const Buffer& GetVertexBuffer() const noexcept { return m_VertexBuffer; }
		[[nodiscard]] const Buffer& GetIndexBuffer() const noexcept { return m_IndexBuffer; }

	private:
		Buffer m_VertexBuffer;
		Buffer m_IndexBuffer;

		std::vector<Vertex> m_Vertices;
		std::vector<u32> m_Indices;
	};
}
