#pragma once

#include <unordered_map>
#include <filesystem>
#include <memory>
#include <concepts>

#include "Component.h"
#include "Log.h"
#include "UUID.h"

namespace Core
{
	class AssetManager
	{
	public:
		~AssetManager();

		template<std::derived_from<Asset> T>
		T* Load(const std::filesystem::path& path);

		template<std::derived_from<Asset> T>
		T* Get(const UUID& id);

		template<std::derived_from<Asset> T>
		std::vector<T*> GetAll();

		Asset* GetAssetByID(const UUID& id);
		std::filesystem::path GetAssetPathByID(const UUID& id);

		[[nodiscard]] const std::unordered_map<std::filesystem::path, std::unique_ptr<Asset>>& GetAllAssets() const noexcept { return m_Assets; }
	private:
		std::unordered_map<std::filesystem::path, std::unique_ptr<Asset>> m_Assets;
	};

	template<std::derived_from<Asset> T>
	T* AssetManager::Load(const std::filesystem::path& path)
	{
		if (m_Assets.contains(path))
		{
			LOG_INFO("Asset: {} already loaded, returning the loaded asset.", path.string());
			return dynamic_cast<T*>(m_Assets[path].get());
		}

		if constexpr (requires(T* asset, const std::filesystem::path& p) { asset->LoadFromFile(p); })
		{
			auto asset = std::make_unique<T>();
			asset->LoadFromFile(path);

			m_Assets[path] = std::move(asset);
			return dynamic_cast<T*>(m_Assets[path].get());
		}
		else
		{
			LOG_DEBUG("Asset class doesn't contain the 'LoadFromFile' function, returning nullptr.");
			return nullptr;
		}
	}

	template<std::derived_from<Asset> T>
	T* AssetManager::Get(const UUID& id)
	{
		for (const auto& [path, asset] : m_Assets)
		{
			if (asset->GetID() == id)
			{
				return dynamic_cast<T*>(asset.get());
			}
		}
		LOG_ERROR("Asset with ID: {} not found.", static_cast<std::string>(id));
		return nullptr;
	}

	template<std::derived_from<Asset> T>
	std::vector<T*> AssetManager::GetAll()
	{
		std::vector<T*> assets;
		for (const auto& [path, asset] : m_Assets)
		{
			if (auto castedAsset = dynamic_cast<T*>(asset.get()); castedAsset)
			{
				assets.push_back(castedAsset);
			}
		}
		return assets;
	}
}