#include "Cube.h"

Cube::Cube(Core::ECS& ecs, const std::string& name):
	Core::Object(ecs, name)
{
	AddComponent<Core::Mesh>();
	auto mesh = GetComponent<Core::Mesh>();  
	*mesh = Core::Application::CreateMeshFromOBJ("Cube.obj");
}

void Cube::OnUpdate(float deltaTime)
{
	auto transform = GetComponent<Core::Transform>();
	transform->Rotation.x += glm::radians(45.0f) * deltaTime;
	transform->Rotation.y += glm::radians(45.0f) * deltaTime;
	transform->Rotation.z += glm::radians(45.0f) * deltaTime;
}