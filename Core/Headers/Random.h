#pragma once

#include <random>

#include "Types.h"

namespace Core
{
	class Random
	{
	public:
		static i32 GetInt(i32 min, i32 max);
		static u32 GetUInt(u32 min, u32 max);

		static f32 GetFloat(f32 min, f32 max);
		static f32 GetFloatNormalized();
	private:
		static inline thread_local auto m_Rng = std::mt19937(std::random_device{}());
		static inline thread_local auto m_FloatNormDist = std::uniform_real_distribution<f32>(0.0f, 1.0f);
	};
}