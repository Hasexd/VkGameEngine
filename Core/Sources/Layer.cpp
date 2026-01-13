#include "Layer.h"
#include "Application.h"

namespace Core
{
	void Layer::QueueTransition(std::unique_ptr<Layer> toLayer)
	{
		// TODO: Queue transition
		auto& layerStack = Application::Get().GetLayerStack();
		for (auto& layer : layerStack)
		{
			if (layer.get() == this)
			{
				layer = std::move(toLayer);
				return;
			}
		}
	}
}