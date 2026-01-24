#pragma once;

#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include <memory>
#include <ranges>

#include "Object.h"
#include "AssetManager.h"
#include "Types.h"

class Project
{
public:
	static Project* Load(const std::filesystem::path& path);

	void Save(const std::vector<std::unique_ptr<Core::Object>>& objects, Core::AssetManager* assetManager) const;

	[[nodiscard]] const std::string& GetName() const { return m_Name; }
	[[nodiscard]] const std::filesystem::path& GetPath() const { return m_Path; }
private:
	std::string m_Name;
	std::filesystem::path m_Path;
};