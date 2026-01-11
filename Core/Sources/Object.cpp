#include "Object.h"

namespace Core
{
	void Object::Bind(VkCommandBuffer commandBuffer)
	{
		m_Mesh.Bind(commandBuffer);
	}

	void Object::Draw(VkCommandBuffer commandBuffer)
	{
		m_Mesh.Draw(commandBuffer);
	}
}