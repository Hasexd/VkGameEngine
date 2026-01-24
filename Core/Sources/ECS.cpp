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

	std::vector<Asset*> ECS::GetAllEntityAssets(const UUID& entity)
	{
		std::vector<Asset*> assets;
		auto it = m_EntityAssetMap.find(entity);

		if (it == m_EntityAssetMap.end())
		{
			LOG_ERROR("Entity {} does not exist.", static_cast<std::string>(entity));
			return assets;
		}

		for (const auto& [typeIdx, assetId] : it->second)
		{
			assets.push_back(m_AssetManager->GetAssetByID(assetId));
		}
		return assets;
	}
}