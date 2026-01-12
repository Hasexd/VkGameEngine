#pragma once

#include "ECS.h"
#include "Transform.h"
#include "Mesh.h"

namespace Core
{
	class Object
	{
	public:
		Object(ECS& ecs);

		template<typename T, typename... Args>
		requires(std::is_base_of_v<Component, T>)
		void AddComponent(Args&&... args);

		template<typename T>
		requires(std::is_base_of_v<Component, T>)
		bool HasComponent();

		template<typename... Ts>
		requires((std::is_base_of_v<Component, Ts> && ...))
		bool HasComponents();

		template<typename T>
		requires(std::is_base_of_v<Component, T>)
		T* GetComponent();

		void Draw(VkCommandBuffer cmd);

	private:
		UUID m_ID;
		ECS& m_ECS;
	};

	template<typename T, typename... Args>
	requires(std::is_base_of_v<Component, T>)
	void Object::AddComponent(Args&&... args)
	{
		m_ECS.AddComponent<T>(m_ID, std::forward<Args>(args)...);
	}

	template<typename T>
	requires(std::is_base_of_v<Component, T>)
	bool Object::HasComponent()
	{
		return m_ECS.HasComponent<T>(m_ID);
	}

	template<typename... Ts>
	requires((std::is_base_of_v<Component, Ts> && ...))
	bool Object::HasComponents()
	{
		return m_ECS.HasComponents<Ts...>(m_ID);
	}

	template<typename T>
	requires(std::is_base_of_v<Component, T>)
	T* Object::GetComponent()
	{
		return m_ECS.GetComponent<T>(m_ID);
	}
}