#pragma once

#include <memory>
#include <concepts>

#include "Types.h"

namespace Core
{
	class Layer
	{
	public:
		Layer() = default;
		virtual ~Layer() = default;

		virtual void OnEvent() {}
		virtual void OnRender() {}
		virtual void OnUpdate(f32 deltaTime) {}

		template<std::derived_from<Layer> T, typename ...Args>
		void TransitionTo(Args&&... args);

	private:
		void QueueTransition(std::unique_ptr<Layer> toLayer);
	};

	template<std::derived_from<Layer> T, typename ...Args>
	void Layer::TransitionTo(Args&&... args)
	{
		QueueTransition(std::move(std::make_unique<T>(std::forward<Args>(args)...)));
	}
}