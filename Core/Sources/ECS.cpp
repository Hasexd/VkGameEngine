#include "ECS.h"

namespace Core
{
	UUID ECS::CreateEntity()
	{
		UUID id;
		m_EntityComponentMap.emplace(id, std::vector<std::unique_ptr<Component>>());
		m_EntityAssetMap.emplace(id, std::unordered_map<std::type_index, UUID>());

		return id;
	}
}