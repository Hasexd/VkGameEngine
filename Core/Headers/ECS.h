#pragma once

#include <unordered_map>
#include <vector>
#include <type_traits>

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

		template<typename T, typename... Args>
		requires(std::is_base_of_v<Component, T>)
		void AddComponent(const UUID& entity, Args&&... args);

		template<typename T>
		requires(std::is_base_of_v<Component, T>)
		bool HasComponent(const UUID& entity);

		template<typename... Ts>
		requires((std::is_base_of_v<Component, Ts> && ...))
		bool HasComponents(const UUID& entity);

		template<typename T>
		requires(std::is_base_of_v<Component, T>)
		T* GetComponent(const UUID& entity);

	private:
		std::unordered_map<UUID, std::vector<std::unique_ptr<Component>>> m_Entities;
 	};

	template<typename T, typename... Args>
	requires(std::is_base_of_v<Component, T>)
	void ECS::AddComponent(const UUID& entity, Args&&... args)
	{
		auto it = m_Entities.find(entity);

		if (it == m_Entities.end())
		{
			LOG_ERROR("Entity {} does not exist.", static_cast<std::string>(entity));
			return;
		}

		if (HasComponent<T>(entity))
		{
			LOG_ERROR("Component of this type already exists on entity: {}", static_cast<std::string>(entity));
			return;
		}

		it->second.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
	}

	template<typename T>
	requires(std::is_base_of_v<Component, T>)
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

	template<typename... Ts>
	requires((std::is_base_of_v<Component, Ts> && ...))
	bool HasComponents(const UUID& entity)
	{
		return (HasComponent<Ts>(entity) && ...);
	}

	template<typename T>
	requires(std::is_base_of_v<Component, T>)
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