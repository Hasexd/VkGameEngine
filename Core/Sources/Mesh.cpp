#include "Mesh.h"

namespace Core
{
	Buffer Mesh::CreateBuffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = memoryUsage;

		Buffer buffer;
		VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.Buffer, &buffer.Allocation, nullptr);
		ASSERT(result == VK_SUCCESS);

		return buffer;
	}

	void Mesh::CopyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion = {};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(queue);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	void Mesh::Create(VmaAllocator allocator, const std::vector<Vertex>& vertices, const std::vector<u32>& indices, 
		VkDevice device, VkCommandPool commandPool, VkQueue transferQueue)
	{
		m_VertexCount = static_cast<u32>(vertices.size());
		m_IndexCount = static_cast<u32>(indices.size());

		VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();

		Buffer stagingBuffer = CreateBuffer(
			allocator,
			vertexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY
		);

		void* data;
		vmaMapMemory(allocator, stagingBuffer.Allocation, &data);
		std::memcpy(data, vertices.data(), vertexBufferSize);
		vmaUnmapMemory(allocator, stagingBuffer.Allocation);

		m_VertexBuffer = CreateBuffer(
			allocator,
			vertexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		CopyBuffer(device, commandPool, transferQueue, stagingBuffer.Buffer, m_VertexBuffer.Buffer, vertexBufferSize);

		vmaDestroyBuffer(allocator, stagingBuffer.Buffer, stagingBuffer.Allocation);

		VkDeviceSize indexBufferSize = sizeof(u32) * indices.size();

		stagingBuffer = CreateBuffer(
			allocator,
			indexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY
		);

		vmaMapMemory(allocator, stagingBuffer.Allocation, &data);
		std::memcpy(data, indices.data(), indexBufferSize);
		vmaUnmapMemory(allocator, stagingBuffer.Allocation);

		m_IndexBuffer = CreateBuffer(
			allocator,
			indexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		CopyBuffer(device, commandPool, transferQueue, stagingBuffer.Buffer, m_IndexBuffer.Buffer, indexBufferSize);

		vmaDestroyBuffer(allocator, stagingBuffer.Buffer, stagingBuffer.Allocation);
	}

	void Mesh::Destroy(VmaAllocator allocator)
	{
		if (m_VertexBuffer.Buffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(allocator, m_VertexBuffer.Buffer, m_VertexBuffer.Allocation);
			m_VertexBuffer.Buffer = VK_NULL_HANDLE;
		}

		if (m_IndexBuffer.Buffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(allocator, m_IndexBuffer.Buffer, m_IndexBuffer.Allocation);
			m_IndexBuffer.Buffer = VK_NULL_HANDLE;
		}

		m_VertexCount = 0;
		m_IndexCount = 0;
	}

	void Mesh::Bind(VkCommandBuffer commandBuffer) const
	{
		std::array vertexBuffers = { m_VertexBuffer.Buffer };
		std::array offsets = { static_cast<VkDeviceSize>(0) };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
		vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
	}

	void Mesh::Draw(VkCommandBuffer commandBuffer) const
	{
		vkCmdDrawIndexed(commandBuffer, m_IndexCount, 1, 0, 0, 0);
	}
}