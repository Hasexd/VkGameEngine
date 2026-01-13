#pragma once

#include "Mesh.h"
#include "Object.h"
#include "Application.h"

class Cube : public Core::Object
{
public:
	Cube(Core::ECS& ecs);

	virtual void OnUpdate(float deltaTime) override;
};