#include "Object.h"

namespace Core
{
	Object::Object(ECS& ecs, const std::string& name):
		m_ECS(ecs),
		m_ID(ecs.CreateEntity()),
		m_Name(name)
	{
		AddComponent<Transform>();
	}
}