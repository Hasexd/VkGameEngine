#pragma once

#include "Mesh.h"
#include "Object.h"
#include "Application.h"

class Plane : public Core::Object
{
public:
	Plane(Core::ECS& ecs, const std::string& name);
};