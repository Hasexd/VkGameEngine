#include "Mesh.h"

namespace Core
{
	Mesh::Mesh(const MeshBuffers& meshBuffers) :
		m_VertexBuffer(meshBuffers.VertexBuffer),
		m_IndexBuffer(meshBuffers.IndexBuffer),
		m_Vertices(meshBuffers.Vertices),
		m_Indices(meshBuffers.Indices)
	{

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

		m_Vertices.clear();
		m_Indices.clear();
	}
	void Mesh::Draw(VkCommandBuffer commandBuffer) const
	{
		VkDeviceSize offset = 0;

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_VertexBuffer.Buffer, &offset);
		vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(commandBuffer, m_Indices.size(), 1, 0, 0, 0);
	}
}