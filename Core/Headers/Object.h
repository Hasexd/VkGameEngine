#pragma once

#include "ECS.h"
#include "Transform.h"

namespace Core
{
	class Object
	{
	public:
		Object(ECS& ecs, const std::string& name);
		virtual ~Object() = default;

		virtual void OnUpdate(float deltaTime) {};

		template<std::derived_from<Component> T, typename... Args>
		T* AddComponent(Args&&... args);

		template<std::derived_from<Asset> T>
		void AddAssetComponent(const UUID& assetId);

		template<std::derived_from<Component> T>
		bool HasComponent();

		template<std::derived_from<Component>... Ts>
		bool HasComponents();

		template<std::derived_from<Component> T>
		[[nodiscard]] T* GetComponent();

		void SetVisible(bool isVisible) { m_IsVisible = isVisible; }
		[[nodiscard]] bool IsVisible() const { return m_IsVisible; }

		[[nodiscard]] const std::string& GetName() const { return m_Name; }
	private:
		UUID m_ID;
		ECS& m_ECS;
		std::string m_Name;
		bool m_IsVisible = true;
	};

	template<std::derived_from<Component> T, typename... Args>
	T* Object::AddComponent(Args&&... args)
	{
		return m_ECS.AddComponent<T>(m_ID, std::forward<Args>(args)...);
	}

	template<std::derived_from<Asset> T>
	void Object::AddAssetComponent(const UUID& assetId)
	{
		m_ECS.AddAssetComponent<T>(m_ID, assetId);
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