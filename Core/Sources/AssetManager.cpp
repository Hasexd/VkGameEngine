#include "AssetManager.h"

namespace Core
{
	AssetManager::~AssetManager()
	{
		m_Assets.clear();
	}

	Asset* AssetManager::GetAssetByID(const UUID& id)
	{
		for (const auto& [path, asset] : m_Assets)
		{
			if (asset->GetID() == id)
			{
				return asset.get();
			}
		}
		LOG_WARN("Asset with ID: {} not found.", static_cast<std::string>(id));
		return nullptr;
	}

	std::filesystem::path AssetManager::GetAssetPathByID(const UUID& id)
	{
		for (const auto& [path, asset] : m_Assets)
		{
			if (asset->GetID() == id)
			{
				return path;
			}
		}
		LOG_WARN("Asset with ID: {} not found.", static_cast<std::string>(id));
		return std::filesystem::path();
	}
}