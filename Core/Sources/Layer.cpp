#include "Layer.h"
#include "Application.h"

namespace Core
{
	void Layer::QueueTransition(std::unique_ptr<Layer> toLayer)
	{
		auto transitionEvent = std::make_unique<TransitionLayerEvent>(this, std::move(toLayer));
		Application::Get().QueuePostFrameEvent(std::move(transitionEvent));
	}
}