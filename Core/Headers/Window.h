#pragma once

#include <memory>
#include <string>

#include <GLFW/glfw3.h>

#include "Types.h"
#include "Log.h"

namespace Core
{
	class Window
	{
	public:
		Window();
		Window(const std::string& title, u32 width, u32 height);

		[[nodiscard]] GLFWwindow* GetNativeWindow() const noexcept { return m_Window.get(); }
		[[nodiscard]] u32 GetWidth() const noexcept { return m_Width; }
		[[nodiscard]] u32 GetHeight() const noexcept { return m_Height; }

	private:
		std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> m_Window;
		u32 m_Width;
		u32 m_Height;
	};
}