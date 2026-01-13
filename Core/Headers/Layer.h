#pragma once

#include <memory>
#include <concepts>

#include "Types.h"
#include "Event.h"
#include "Log.h"

namespace Core
{
	class Layer
	{
	public:
		Layer() = default;
		virtual ~Layer() = default;

		virtual void OnEvent(Event& event) {}
		virtual void OnRender() {}
		virtual void OnSwapchainRender() {}
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

	class TransitionLayerEvent : public Event
	{
	public:
		TransitionLayerEvent(Layer* fromLayer, std::unique_ptr<Layer> toLayer): 
			m_FromLayer(fromLayer), m_ToLayer(std::move(toLayer)) {}

		inline std::unique_ptr<Layer> Transition() { return std::move(m_ToLayer); }
		inline Layer* GetFromLayer() const { return m_FromLayer; }

		std::string ToString() const override
		{
			return std::format("TransitionLayerEvent to {}", m_ToLayer ? typeid(*m_ToLayer).name() : "nullptr");
		}

		EVENT_CLASS_TYPE(TransitionLayer)
	private:
		Layer* m_FromLayer;
		std::unique_ptr<Layer> m_ToLayer;
	};
}