#pragma once

#include "Mesh.h"
#include "Object.h"
#include "Application.h"
#include "AssetManager.h"

class Cube : public Core::Object
{
public:
	Cube(Core::ECS& ecs, const std::string& name, Core::AssetManager* assetManager);
};