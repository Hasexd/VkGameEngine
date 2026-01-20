#include "Material.h"


namespace Core
{
	void Material::LoadFromFile(const std::filesystem::path& path)
	{
		std::ifstream file(path);

		if (!file.is_open())
		{
			LOG_ERROR("Couldn't find the .mtl file: {}", path.string());
			return;
		}

		Ambient = glm::vec3(0.2f);
		Diffuse = glm::vec3(0.8f);
		Specular = glm::vec3(1.0f);
		Shininess = 32.0f;

		std::string line;
		while (std::getline(file, line))
		{
			line.erase(0, line.find_first_not_of(" \t\r\n"));
			line.erase(line.find_last_not_of(" \t\r\n") + 1);

			if (line.empty() || line[0] == '#')
				continue;

			std::istringstream iss(line);
			std::string prefix;
			iss >> prefix;

			if (prefix == "Ka")
			{
				float r, g, b;
				iss >> r >> g >> b;
				Ambient = glm::vec3(r, g, b);
			}
			else if (prefix == "Kd")
			{
				float r, g, b;
				iss >> r >> g >> b;
				Diffuse = glm::vec3(r, g, b);
			}
			else if (prefix == "Ks")
			{
				float r, g, b;
				iss >> r >> g >> b;
				Specular = glm::vec3(r, g, b);
			}
			else if (prefix == "Ns")
			{
				iss >> Shininess;
			}
		}

		file.close();
	}
}