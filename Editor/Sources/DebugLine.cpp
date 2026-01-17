#include "DebugLine.h"

DebugLine::DebugLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, f32 lifetime, f32 thickness) :
	Lifetime(lifetime), Thickness(thickness), m_Color(color)
{
	CreateBuffers(start, end);
	s_Instances++;
}

DebugLine::~DebugLine()
{
	vmaDestroyBuffer(Core::Application::Get().GetVmaAllocator(), m_VertexBuffer.Buffer, m_VertexBuffer.Allocation);

	s_Instances--;
	if (s_Instances == 0)
	{
		vmaDestroyBuffer(Core::Application::Get().GetVmaAllocator(), s_IndexBuffer.Buffer, s_IndexBuffer.Allocation);
	}
}

void DebugLine::Draw() const
{
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(Core::Application::Get().GetCurrentCommandBuffer(), 0, 1, &m_VertexBuffer.Buffer, &offset);
	vkCmdBindIndexBuffer(Core::Application::Get().GetCurrentCommandBuffer(), s_IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(Core::Application::Get().GetCurrentCommandBuffer(), 2, 1, 0, 0, 0);
}

void DebugLine::CreateBuffers(const glm::vec3& start, const glm::vec3& end)
{
	auto& app = Core::Application::Get();

	std::array<glm::vec3, 2> vertices = { start, end, };

	Core::Buffer stagingBuffer = app.CreateBuffer(
		sizeof(glm::vec3) * vertices.size(),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY
	);

	m_VertexBuffer = app.CreateBuffer(sizeof(glm::vec3) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	void* data;
	vmaMapMemory(app.GetVmaAllocator(), stagingBuffer.Allocation, &data);
	std::memcpy(data, vertices.data(), sizeof(glm::vec3) * vertices.size());
	vmaUnmapMemory(app.GetVmaAllocator(), stagingBuffer.Allocation);

	app.CopyBuffer(stagingBuffer.Buffer, m_VertexBuffer.Buffer, sizeof(glm::vec3) * vertices.size());
	vmaDestroyBuffer(app.GetVmaAllocator(), stagingBuffer.Buffer, stagingBuffer.Allocation);

	if (!s_Instances)
	{
		std::array<u32, 2> indices = { 0, 1 };

		stagingBuffer = app.CreateBuffer(
			sizeof(u32) * indices.size(),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY
		);

		s_IndexBuffer = app.CreateBuffer(sizeof(u32) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

		vmaMapMemory(app.GetVmaAllocator(), stagingBuffer.Allocation, &data);
		std::memcpy(data, indices.data(), sizeof(u32) * indices.size());
		vmaUnmapMemory(app.GetVmaAllocator(), stagingBuffer.Allocation);

		Core::Application::Get().CopyBuffer(stagingBuffer.Buffer, s_IndexBuffer.Buffer, sizeof(u32) * indices.size());
		vmaDestroyBuffer(app.GetVmaAllocator(), stagingBuffer.Buffer, stagingBuffer.Allocation);
	}
}