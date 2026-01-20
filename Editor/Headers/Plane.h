#pragma once

#include "Mesh.h"
#include "Object.h"
#include "AssetManager.h"

class Plane : public Core::Object
{
public:
	Plane(Core::ECS& ecs, const std::string& name, Core::AssetManager* assetManager);
};