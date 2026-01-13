#pragma once

#include <memory>
#include <vector>

#include <GLFW/glfw3.h>

#include "Application.h"
#include "Layer.h"
#include "ECS.h"
#include "Camera.h"
#include "Object.h"
#include "Mesh.h"

class Editor : public Core::Layer
{
public:
	Editor();
	virtual void OnEvent() override;
	virtual void OnRender() override;
	virtual void OnUpdate(f32 deltaTime) override;

	std::shared_ptr<Core::Object> AddObject();
	void CreateMesh(const std::shared_ptr<Core::Object>& obj, const std::vector<Core::Vertex>& vertices, const std::vector<u32>& indices);

private:
	Core::ECS m_ECS;
	Core::Camera m_Camera;

	std::vector<std::shared_ptr<Core::Object>> m_Objects;
};