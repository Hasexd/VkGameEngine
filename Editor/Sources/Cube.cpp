#include "Cube.h"

Cube::Cube(Core::ECS& ecs, const std::string& name):
	Core::Object(ecs, name)
{
	auto mesh = AddComponent<Core::Mesh>();
	*mesh = Core::Application::CreateMeshFromOBJ("Cube.obj");
}
