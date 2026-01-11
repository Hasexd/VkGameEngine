#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <random>

#include "Types.h"

namespace Core
{
	class UUID
	{
	public:
		UUID();

		[[nodiscard]] operator std::string() const noexcept { return m_UUID; }
	private:
		std::string m_UUID;
	};
}