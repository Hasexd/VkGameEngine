#pragma once

#include "Window.h"

namespace Core
{
	class Application
	{
	public:
		Application(const std::string& title, u32 width, u32 height);

		void Run();

	private:
		Window m_Window;
	};
}