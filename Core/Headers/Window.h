#pragma once

#include <string>
#include <memory>

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#include "Types.h"
#include "Log.h"
#include "WindowEvents.h"
#include "InputEvents.h"

namespace Core
{
	class Window
	{
		using EventCallbackFn = std::function<void(Event&)>;
	public:
		Window();
		~Window();

		void Create(const std::string& title, u32 width, u32 height);
		void Destroy();

		void SetEventCallback(const EventCallbackFn& callback) { m_EventCallback = callback; }
		void RaiseEvent(Event& event);

		glm::vec2 GetFramebufferSize() const;
		glm::vec2 GetMousePos() const;

		bool ShouldClose() const;

		GLFWwindow* GetHandle() const { return m_Handle.get(); }
		void SetTitle(const std::string& title);

	private:
		std::string m_Title;
		u32 m_Width;
		u32 m_Height;
		std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> m_Handle;

		EventCallbackFn m_EventCallback;
	};
}