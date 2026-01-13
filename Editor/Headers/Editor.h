#pragma once

#include <memory>
#include <vector>
#include <unordered_set>

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
	virtual void OnEvent(Core::Event& event) override;
	virtual void OnRender() override;
	virtual void OnUpdate(f32 deltaTime) override;

	bool OnMouseButtonPressed(Core::MouseButtonPressedEvent& event);
	bool OnMouseMoved(Core::MouseMovedEvent& event);
	bool OnKeyPressed(Core::KeyPressedEvent& event);
	bool OnKeyReleased(Core::KeyReleasedEvent& event);


	std::shared_ptr<Core::Object> AddObject();
	void CreateMesh(const std::shared_ptr<Core::Object>& obj, const std::vector<Core::Vertex>& vertices, const std::vector<u32>& indices);

private:
	Core::ECS m_ECS;
	Core::Camera m_Camera;

	f64 m_LastMouseX = 0.0;
	f64 m_LastMouseY = 0.0;

	std::vector<std::shared_ptr<Core::Object>> m_Objects;
	std::unordered_set<i32> m_PressedKeys;
};