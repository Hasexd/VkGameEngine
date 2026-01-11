#include "Random.h"


namespace Core
{
	i32 Random::GetInt(i32 min, i32 max)
	{
		std::uniform_int_distribution<i32> dist(min, max);
		return dist(m_Rng);
	}

	u32 Random::GetUInt(u32 min, u32 max)
	{
		std::uniform_int_distribution<u32> dist(min, max);
		return dist(m_Rng);
	}

	f32 Random::GetFloat(f32 min, f32 max)
	{
		std::uniform_real_distribution<f32> dist(min, max);
		return dist(m_Rng);
	}

	f32 Random::GetFloatNormalized()
	{
		return m_FloatNormDist(m_Rng);
	}
}