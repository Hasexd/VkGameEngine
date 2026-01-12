#include "ECS.h"

namespace Core
{
	UUID ECS::CreateEntity()
	{
		UUID id;
		m_Entities.emplace(id, std::vector<std::unique_ptr<Component>>());
		return id;
	}
}