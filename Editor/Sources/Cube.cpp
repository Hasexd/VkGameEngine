#include "Cube.h"

Cube::Cube(Core::ECS& ecs):
	Core::Object(ecs)
{
	AddComponent<Core::Mesh>();
	auto mesh = GetComponent<Core::Mesh>();  
	*mesh = Core::Application::CreateMeshFromOBJ("Cube.obj");
}