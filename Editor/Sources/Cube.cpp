#include "Cube.h"

Cube::Cube(Core::ECS& ecs, const std::string& name):
	Core::Object(ecs, name)
{
	AddComponent<Core::Mesh>();
	auto mesh = GetComponent<Core::Mesh>();  
	*mesh = Core::Application::CreateMeshFromOBJ("Cube.obj");
}
