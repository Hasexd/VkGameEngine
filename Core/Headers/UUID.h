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

		bool operator==(const UUID& other) const noexcept
		{
			return m_UUID == other.m_UUID;
		}
	private:
		std::string m_UUID;
	};
}

template<>
struct std::hash<Core::UUID>
{
	std::size_t operator()(const Core::UUID& uuid) const noexcept
	{
		return std::hash<std::string>()(static_cast<std::string>(uuid));
	}
};
