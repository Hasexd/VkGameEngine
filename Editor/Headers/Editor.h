#pragma once

#include <memory>
#include <vector>
#include <unordered_set>

#include <GLFW/glfw3.h>

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>
#include <imgui.h>

#include "Application.h"
#include "Layer.h"
#include "ECS.h"
#include "Camera.h"
#include "Object.h"
#include "Mesh.h"
#include "Cube.h"
#include "Material.h"
#include "VkTypes.h"

class Editor : public Core::Layer
{
public:
	Editor();
	virtual ~Editor() override;
	virtual void OnEvent(Core::Event& event) override;
	virtual void OnRender() override;
	virtual void OnSwapchainRender() override;
	virtual void OnUpdate(f32 deltaTime) override;

	bool OnMouseButtonPressed(Core::MouseButtonPressedEvent& event);
	bool OnMouseMoved(Core::MouseMovedEvent& event);
	bool OnKeyPressed(Core::KeyPressedEvent& event);
	bool OnKeyReleased(Core::KeyReleasedEvent& event);
	bool OnWindowResize(Core::WindowResizeEvent& event);

	template<std::derived_from<Core::Object> T, typename... Args>
	T* AddObject(const std::string& name, Args&&... args);

private:
	void InitImGui();
	void RenderImGui();

	void UpdateVPData();
	void PushConstants(Core::Object* obj);

	void RenderObjects();
	
	void CreateOutlinePipeline();
private:
	Core::ECS m_ECS;
	Core::Camera m_Camera;

	f64 m_LastMouseX = 0.0;
	f64 m_LastMouseY = 0.0;

	Core::Object* m_SelectedObject = nullptr;
	std::vector<std::unique_ptr<Core::Object>> m_Objects;
	std::unordered_set<i32> m_PressedKeys;

	VkDescriptorPool m_ImGuiDescriptorPool;

	std::filesystem::path m_ShaderDirectory = std::filesystem::path(PATH_TO_SHADERS);

	Core::Shader m_OutlineShader;
};

template<std::derived_from<Core::Object> T, typename... Args>
T* Editor::AddObject(const std::string& name, Args&&... args)
{
	auto& ptr = m_Objects.emplace_back(std::make_unique<T>(m_ECS, name, std::forward<Args>(args)...));
	return static_cast<T*>(ptr.get());
}