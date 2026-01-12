#include "Object.h"

namespace Core
{
	Object::Object(ECS& ecs):
		m_ECS(ecs),
		m_ID(ecs.CreateEntity()) 
	{
		AddComponent<Transform>();
	}

	void Object::Draw(VkCommandBuffer cmd)
	{
		Mesh* mesh = GetComponent<Mesh>();

		mesh->Bind(cmd);
		mesh->Draw(cmd);
	}
}