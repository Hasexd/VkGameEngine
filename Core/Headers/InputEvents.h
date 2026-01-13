#pragma once

#include <format>

#include "Event.h"
#include "Types.h"

namespace Core
{
	class KeyEvent : public Event
	{
	public:
		inline i32 GetKeyCode() const { return m_KeyCode; }
	protected:
		KeyEvent(i32 keycode):
			m_KeyCode(keycode) {}

		i32 m_KeyCode;
	};

	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(i32 keycode, bool isRepeat):
			KeyEvent(keycode), m_IsRepeat(isRepeat) { }
		
		inline bool IsRepeat() const { return m_IsRepeat; }

		std::string ToString() const override
		{
			return std::format("KeyPressedEvent: {} (repeat={})", m_KeyCode, m_IsRepeat);
		}

		EVENT_CLASS_TYPE(KeyPressed)
	private:
		bool m_IsRepeat;
	};

	class KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(i32 keycode) :
			KeyEvent(keycode) {
		}

		std::string ToString() const override
		{
			return std::format("KeyReleasedEvent: {}", m_KeyCode);
		}

		EVENT_CLASS_TYPE(KeyReleased)
	};

	class MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent(f64 x, f64 y):
			m_MouseX(x), m_MouseY(y) { }

		inline f64 GetX() const { return m_MouseX; }
		inline f64 GetY() const { return m_MouseY; }

		std::string ToString() const override
		{
			return std::format("MouseMovedEvent: {}, {}", m_MouseX, m_MouseY);
		}
		
		EVENT_CLASS_TYPE(MouseMoved)
	private:
		f64 m_MouseX, m_MouseY;
	};

	class MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent(f64 xOffset, f64 yOffset) :
			m_OffsetX(xOffset), m_OffsetY(yOffset) {
		}

		inline f64 GetOffsetX() const { return m_OffsetX; }
		inline f64 GetOfsetY() const { return m_OffsetY; }

		std::string ToString() const override
		{
			return std::format("MouseScrolledEvent: {}, {}", m_OffsetX, m_OffsetY);
		}

		EVENT_CLASS_TYPE(MouseScrolled)
	private:
		f64 m_OffsetX, m_OffsetY;
	};

	class MouseButtonEvent : public Event
	{
	public:
		inline i32 GetMouseButton() const { return m_Button; }
	protected:
		MouseButtonEvent(i32 button)
			: m_Button(button) {
		}

		i32 m_Button;
	};

	class MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(i32 button)
			: MouseButtonEvent(button) {
		}

		std::string ToString() const override
		{
			return std::format("MouseButtonPressedEvent: {}", m_Button);
		}

		EVENT_CLASS_TYPE(MouseButtonPressed)
	};

	class MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(i32 button)
			: MouseButtonEvent(button) {
		}

		std::string ToString() const override
		{
			return std::format("MouseButtonReleasedEvent: {}", m_Button);
		}

		EVENT_CLASS_TYPE(MouseButtonReleased)
	};
}