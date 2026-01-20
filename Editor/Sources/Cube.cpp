#include "Cube.h"

Cube::Cube(Core::ECS& ecs, const std::string& name, Core::AssetManager* assetManager):
	Core::Object(ecs, name)
{
	auto mesh = assetManager->Load<Core::Mesh>(std::filesystem::path(PATH_TO_OBJS) / "Cube.obj");
	AddAssetComponent<Core::Mesh>(mesh->GetID());
}
