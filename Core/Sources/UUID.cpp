#include "UUID.h"


namespace Core
{
	UUID::UUID()
	{
		std::random_device rd;
		std::mt19937_64 gen(rd());
		std::uniform_int_distribution<uint32_t> dis(0, 15);
		std::uniform_int_distribution<uint32_t> dis2(8, 11);

		std::stringstream ss;
		ss << std::hex << std::setfill('0');

		for (int i = 0; i < 8; i++) {
			ss << dis(gen);
		}
		ss << "-";

		for (int i = 0; i < 4; i++) {
			ss << dis(gen);
		}
		ss << "-";

		ss << "4";
		for (int i = 0; i < 3; i++) {
			ss << dis(gen);
		}
		ss << "-";

		ss << dis2(gen);
		for (int i = 0; i < 3; i++) {
			ss << dis(gen);
		}
		ss << "-";

		for (int i = 0; i < 12; i++) {
			ss << dis(gen);
		}

		m_UUID = ss.str();
	}
}