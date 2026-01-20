#pragma once

#include <unordered_map>
#include <vector>
#include <concepts>
#include <memory>

#include "UUID.h"
#include "Types.h"
#include "Random.h"
#include "Component.h"
#include "Log.h"

namespace Core
{
	class ECS
	{
	public:
		UUID CreateEntity();

		template<std::derived_from<Component> T, typename... Args>
		T* AddComponent(const UUID& entity, Args&&... args);

		template<std::derived_from<Component> T>
		bool HasComponent(const UUID& entity);

		template<std::derived_from<Component>... Ts>
		bool HasComponents(const UUID& entity);

		template<std::derived_from<Component> T>
		T* GetComponent(const UUID& entity);

	private:
		std::unordered_map<UUID, std::vector<std::unique_ptr<Component>>> m_Entities;
 	};

	template<std::derived_from<Component> T, typename... Args>
	T* ECS::AddComponent(const UUID& entity, Args&&... args)
	{
		auto it = m_Entities.find(entity);

		if (it == m_Entities.end())
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

	template<std::derived_from<Component> T>
	bool ECS::HasComponent(const UUID& entity)
	{
		auto it = m_Entities.find(entity);

		if (it == m_Entities.end())
		{
			LOG_ERROR("Entity {} does not exist.", static_cast<std::string>(entity));
			return false;
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
		auto it = m_Entities.find(entity);

		if (it == m_Entities.end())
		{
			LOG_ERROR("Entity {} does not exist.", static_cast<std::string>(entity));
			return nullptr;
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