#pragma once

#include <format>

#include "Event.h"
#include "Types.h"

namespace Core {

	class WindowClosedEvent : public Event
	{
	public:
		WindowClosedEvent() {}

		EVENT_CLASS_TYPE(WindowClose)
	};

	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(u32 width, u32 height)
			: m_Width(width), m_Height(height) {
		}

		inline u32 GetWidth() const { return m_Width; }
		inline u32 GetHeight() const { return m_Height; }

		std::string ToString() const override
		{
			return std::format("WindowResizeEvent: {}, {}", m_Width, m_Height);
		}

		EVENT_CLASS_TYPE(WindowResize)
	private:
		u32 m_Width, m_Height;
	};
}