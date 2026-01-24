#include "Plane.h"

Plane::Plane(Core::ECS& ecs, const std::string& name, Core::AssetManager* assetManager) :
	Core::Object(ecs, name)
{
	auto mesh = assetManager->Load<Core::Mesh>(std::filesystem::path(PATH_TO_OBJS) / "Plane.obj");
	AddComponent<Core::Mesh>(mesh->GetID());
}