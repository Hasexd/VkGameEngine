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

		// for testing purposes
		template<std::derived_from<Asset> T, typename... Args>
		T* Create(Args&&... args);

		template<std::derived_from<Asset> T>
		T* Load(const std::filesystem::path& path);

		template<std::derived_from<Asset> T>
		T* Get(const UUID& id);
	private:
		std::unordered_map<std::filesystem::path, std::unique_ptr<Asset>> m_Assets;
	};

	template<std::derived_from<Asset> T, typename... Args>
	T* AssetManager::Create(Args&&... args)
	{
		auto asset = std::make_unique<T>(std::forward<Args>(args)...);
		T* assetPtr = asset.get();
		m_Assets[std::filesystem::path()] = std::move(asset);
		return assetPtr;
	}

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
}