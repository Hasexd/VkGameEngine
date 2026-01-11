#pragma once

#include <vector>
#include <array>

#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>

#include "Types.h"
#include "VkTypes.h"
#include "Log.h"

namespace Core
{
	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Color;
	};

	class Mesh
	{
	public:
		Mesh() = default;
		~Mesh() = default;

		void Create(VmaAllocator allocator, const std::vector<Vertex>& vertices, const std::vector<u32>& indices,
			VkDevice device, VkCommandPool commandPool, VkQueue transferQueue);
		void Destroy(VmaAllocator allocator);

		void Bind(VkCommandBuffer commandBuffer) const;
		void Draw(VkCommandBuffer commandBuffer) const;

		[[nodiscard]] u32 GetIndexCount() const noexcept { return m_IndexCount; }
		[[nodiscard]] u32 GetVertexCount() const noexcept { return m_VertexCount; }

	private:
		Buffer CreateBuffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
		void CopyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	private:
		Buffer m_VertexBuffer;
		Buffer m_IndexBuffer;
		u32 m_VertexCount = 0;
		u32 m_IndexCount = 0;
	};
}
