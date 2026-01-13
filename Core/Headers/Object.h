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
		virtual ~Object() = default;

		virtual void OnUpdate(float deltaTime) {};

		template<std::derived_from<Component> T, typename... Args>
		void AddComponent(Args&&... args);

		template<std::derived_from<Component> T>
		bool HasComponent();

		template<std::derived_from<Component>... Ts>
		bool HasComponents();

		template<std::derived_from<Component> T>
		T* GetComponent();

		void Draw(VkCommandBuffer cmd);

	private:
		UUID m_ID;
		ECS& m_ECS;
	};

	template<std::derived_from<Component> T, typename... Args>
	void Object::AddComponent(Args&&... args)
	{
		m_ECS.AddComponent<T>(m_ID, std::forward<Args>(args)...);
	}

	template<std::derived_from<Component> T>
	bool Object::HasComponent()
	{
		return m_ECS.HasComponent<T>(m_ID);
	}

	template<std::derived_from<Component>... Ts>
	bool Object::HasComponents()
	{
		return m_ECS.HasComponents<Ts...>(m_ID);
	}

	template<std::derived_from<Component> T>
	T* Object::GetComponent()
	{
		return m_ECS.GetComponent<T>(m_ID);
	}
}