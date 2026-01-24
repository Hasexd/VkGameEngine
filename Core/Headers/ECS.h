#pragma once

#include <unordered_map>
#include <vector>
#include <concepts>
#include <memory>
#include <typeindex>

#include "UUID.h"
#include "Types.h"
#include "Random.h"
#include "Component.h"
#include "Log.h"
#include "AssetManager.h"

namespace Core
{
	class ECS
	{
	public:
		explicit ECS(AssetManager* assetManager) : m_AssetManager(assetManager) {}
		UUID CreateEntity();

		template<std::derived_from<Component> T, typename... Args>
		T* AddComponent(const UUID& entity, Args&&... args);

		template<std::derived_from<Asset> T>
		void AddComponent(const UUID& entity, const UUID& assetId);

		template<std::derived_from<Component> T>
		bool HasComponent(const UUID& entity);

		template<std::derived_from<Component>... Ts>
		bool HasComponents(const UUID& entity);

		template<std::derived_from<Component> T>
		T* GetComponent(const UUID& entity);

		std::vector<Asset*> GetAllEntityAssets(const UUID& entity);
	private:
		std::unordered_map<UUID, std::vector<std::unique_ptr<Component>>> m_EntityComponentMap;
		std::unordered_map<UUID, std::unordered_map<std::type_index, UUID>> m_EntityAssetMap;
		AssetManager* m_AssetManager = nullptr;
 	};

	template<std::derived_from<Component> T, typename... Args>
	T* ECS::AddComponent(const UUID& entity, Args&&... args)
	{
		auto it = m_EntityComponentMap.find(entity);

		if (it == m_EntityComponentMap.end())
		{
			LOG_ERROR("Entity {} does not exist.", static_cast<std::string>(entity));
			return nullptr;
		}

		if (HasComponent<T>(entity))
		{
			LOG_ERROR("Component of this type already exists on entity: {}", static_cast<std::string>(entity));
			return nullptr;
		}

		Component* component = it->second.emplace_back(std::make_unique<T>(std::forward<Args>(args)...)).get();
		return dynamic_cast<T*>(component);
	}

	template<std::derived_from<Asset> T>
	void ECS::AddComponent(const UUID& entity, const UUID& assetId)
	{
		auto it = m_EntityAssetMap.find(entity);

		if (it == m_EntityAssetMap.end())
		{
			LOG_ERROR("Entity {} does not exist.", static_cast<std::string>(entity));
			return;
		}

		if (it->second.contains(std::type_index(typeid(T))))
		{
			LOG_ERROR("Asset of this type already exists on entity: {}", static_cast<std::string>(entity));
			return;
		}

		Asset* asset = m_AssetManager->Get<T>(assetId);
		m_EntityAssetMap[entity][typeid(T)] = assetId;
	}

	template<std::derived_from<Component> T>
	bool ECS::HasComponent(const UUID& entity)
	{
		auto it = m_EntityComponentMap.find(entity);

		if (it == m_EntityComponentMap.end())
		{
			LOG_ERROR("Entity {} does not exist.", static_cast<std::string>(entity));
			return false;
		}

		if constexpr (std::derived_from<T, Asset>)
		{
			return m_EntityAssetMap.contains(entity) && m_EntityAssetMap[entity].contains(std::type_index(typeid(T)));
		}
		
		for (const auto& component : it->second)
		{
			if (dynamic_cast<T*>(component.get()))
			{
				return true;
			}
		}

		return false;
	}

	template<std::derived_from<Component>... Ts>
	bool ECS::HasComponents(const UUID& entity)
	{
		return (HasComponent<Ts>(entity) && ...);
	}

	template<std::derived_from<Component> T>
	T* ECS::GetComponent(const UUID& entity)
	{
		auto it = m_EntityComponentMap.find(entity);

		if (it == m_EntityComponentMap.end())
		{
			LOG_ERROR("Entity {} does not exist.", static_cast<std::string>(entity));
			return nullptr;
		}

		if constexpr (std::derived_from<T, Asset>)
		{
			return m_AssetManager->Get<T>(m_EntityAssetMap[entity][std::type_index(typeid(T))]);
		}

		for (const auto& component : it->second)
		{
			if (auto castedComponent = dynamic_cast<T*>(component.get()))
			{
				return castedComponent;
			}
		}

		LOG_ERROR("Component of this type does not exist on entity: {}", static_cast<std::string>(entity));

		return nullptr;
	}

	
}