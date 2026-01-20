#pragma once

#include "UUID.h"

namespace Core
{
	class Component { public: virtual ~Component() = default; };

	class Asset : public Component 
	{ 
	public: 
		Asset() : m_ID() {}
		virtual ~Asset() override = default;

		[[nodiscard]] const UUID& GetID() const noexcept { return m_ID; }

	private:
		UUID m_ID;
	};
}