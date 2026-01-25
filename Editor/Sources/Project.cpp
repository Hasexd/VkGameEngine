#include "Project.h"

namespace
{
	// note: this function will return false if the paths match (https://stackoverflow.com/questions/62503197/check-if-path-contains-another-in-c)
	bool IsSubpath(const std::filesystem::path& path, const std::filesystem::path& base)
	{
		auto rel = std::filesystem::relative(path, base);
		return !rel.empty() && rel.native()[0] != '.';
	}
}


Project* Project::Load(const std::filesystem::path& path)
{
	Project* project = new Project();
	project->m_Path = path.parent_path();
	project->m_Name = path.stem().string();

	return project;
}

/*
* .Content file structure:
* [Header]
* - Object Count [4 bytes]
* 
* [Objects]
* - Name length [4 bytes]
* - Name string
* - Position [12 bytes]
* - Rotation [12 bytes]
* - Scale [12 bytes]
* - Asset Count [4 bytes]
* 
* - For each asset: 
*	- Asset path length [4 bytes]
*	- Asset paths
*/

void Project::Save(const std::vector<std::unique_ptr<Core::Object>>& objects, Core::AssetManager* assetManager) const
{
	std::fstream contentFile(m_Path / ".Content", std::ios::out | std::ios::binary);

	u32 objectCount = static_cast<u32>(objects.size());

	contentFile.write(reinterpret_cast<char*>(&objectCount), sizeof(u32));

	std::string name, assetPath;
	u32 nameLength = 0, assetCount = 0, assetPathLength = 0;
	Core::Transform* transform = nullptr;
	std::vector<Core::Asset*> assets;

	for (const auto& object : objects)
	{
		name = object->GetName();
		nameLength = static_cast<u32>(name.size());
		transform = object->GetComponent<Core::Transform>();
		assets = object->GetAllAssets();
		assetCount = static_cast<u32>(assets.size());

		contentFile.write(reinterpret_cast<char*>(&nameLength), sizeof(u32));
		contentFile.write(name.c_str(), nameLength);
		contentFile.write(reinterpret_cast<char*>(&transform->Position), sizeof(glm::vec3));
		contentFile.write(reinterpret_cast<char*>(&transform->Rotation), sizeof(glm::vec3));
		contentFile.write(reinterpret_cast<char*>(&transform->Scale), sizeof(glm::vec3));
		contentFile.write(reinterpret_cast<char*>(&assetCount), sizeof(u32));

		for (const auto& asset : assets)
		{
			assetPath = assetManager->GetAssetPathByID(asset->GetID()).string();

			// save relative paths if the asset is inside the project directory
			if (IsSubpath(assetPath, m_Path))
			{
				assetPath = std::filesystem::relative(assetPath, m_Path).string();
			}

			assetPathLength = static_cast<u32>(assetPath.size());
			contentFile.write(reinterpret_cast<char*>(&assetPathLength), sizeof(u32));
			contentFile.write(assetPath.c_str(), assetPathLength);
		}
	}
	contentFile.close();
}