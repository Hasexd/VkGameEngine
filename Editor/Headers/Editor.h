#pragma once

#include <memory>
#include <vector>
#include <unordered_set>
#include <numeric>

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
#include "DebugLine.h"
#include "Ray.h"
#include "Plane.h"
#include "Gizmo.h"

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
	bool OnMouseButtonReleased(Core::MouseButtonReleasedEvent& event);
	bool OnMouseMoved(Core::MouseMovedEvent& event);
	bool OnKeyPressed(Core::KeyPressedEvent& event);
	bool OnKeyReleased(Core::KeyReleasedEvent& event);
	bool OnWindowResize(Core::WindowResizeEvent& event);

	static void DrawDebugLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, f32 lifetime, f32 thickness);

	template<std::derived_from<Core::Object> T, typename... Args>
	T* AddObject(const std::string& name, Args&&... args);

	Core::HitResult Raycast(const glm::vec3& start, const glm::vec3& direction, f32 maxDistance);

private:
	void InitImGui();
	void InitGizmos();
	void RenderImGui();

	void UpdateVPData();
	void PushConstants(Core::Object* obj);

	void RenderObjects(Core::Application& app);
	void RenderGizmos(Core::Application& app);
	void RenderSelectedObjectOutline(Core::Application& app);
	void RenderDebugLines(Core::Application& app);
	
	void CreateOutlinePipeline();
	void CreateDebugLinePipeline();
	void CreateGizmoPipeline();

	bool RayTriangleIntersection(const Core::Ray& ray, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, f32& outDistance);
	Core::HitResult GizmoRaycast(const glm::vec3& start, const glm::vec3& direction, f32 maxDistance);

	template<std::derived_from<Core::Object> T>
	Core::HitResult RaycastInternal(const glm::vec3& start, const glm::vec3& direction, f32 maxDistance, const std::vector<std::unique_ptr<T>>& objects);

	bool TestGizmoClick();
	bool TestObjectClick();

	Core::Ray GetMouseRay();


	// Gizmo manipulation methods taken from TinyGizmos implementation (https://github.com/ddiakopoulos/tinygizmo)
	void PlaneTranslationDragger(const glm::vec3& planeNormal, glm::vec3& point);
	void AxisTranslationDragger(const glm::vec3& axis, glm::vec3& point);
private:
	Core::ECS m_ECS;
	Core::Camera m_Camera;

	f64 m_LastMouseX = 0.0;
	f64 m_LastMouseY = 0.0;

	std::vector<std::unique_ptr<Core::Object>> m_Objects;
	Core::Object* m_SelectedObject = nullptr;


	std::vector<std::unique_ptr<Gizmo>> m_Gizmos;
	GizmoType m_ActiveGizmoType = GizmoType::Translate;
	Gizmo* m_ActiveGizmo = nullptr;

	std::unordered_set<i32> m_PressedKeys;

	VkDescriptorPool m_ImGuiDescriptorPool;

	std::filesystem::path m_ShaderDirectory = std::filesystem::path(PATH_TO_SHADERS);

	Core::Shader m_OutlineShader;
	Core::Shader m_OutlineFillShader;
	Core::Shader m_DebugLineShader;
	Core::Shader m_GizmoShader;

	static inline std::vector<std::unique_ptr<DebugLine>> m_DebugLines;

	// do not modify
	static inline f32 s_MaxLineWidth = 1.0f;

	bool m_WireframeMode = false;
};

template<std::derived_from<Core::Object> T, typename... Args>
T* Editor::AddObject(const std::string& name, Args&&... args)
{
	auto& ptr = m_Objects.emplace_back(std::make_unique<T>(m_ECS, name, std::forward<Args>(args)...));
	return static_cast<T*>(ptr.get());
}

template<std::derived_from<Core::Object> T>
Core::HitResult Editor::RaycastInternal(const glm::vec3& start, const glm::vec3& direction, f32 maxDistance, const std::vector<std::unique_ptr<T>>& objects)
{
	f32 closestDistance = std::numeric_limits<f32>::max();
	Core::Object* closestObject = nullptr;

	for (const auto& obj : objects)
	{
		if (!obj->HasComponent<Core::Mesh>())
			continue;

		auto transform = obj->GetComponent<Core::Transform>();

		glm::mat4 invTransform = glm::inverse(transform->GetModelMatrix());
		glm::vec3 localOrigin = glm::vec3(invTransform * glm::vec4(start, 1.0f));
		glm::vec3 localDirection = glm::normalize(glm::vec3(invTransform * glm::vec4(direction, 0.0f)));

		auto mesh = obj->GetComponent<Core::Mesh>();
		const auto& vertices = mesh->GetVertices();
		const auto& indices = mesh->GetIndices();

		for (usize i = 0; i < indices.size(); i += 3)
		{
			if (indices[i] >= vertices.size() ||
				indices[i + 1] >= vertices.size() ||
				indices[i + 2] >= vertices.size())
			{
				continue;
			}

			glm::vec3 v0 = vertices[indices[i]].Position;
			glm::vec3 v1 = vertices[indices[i + 1]].Position;
			glm::vec3 v2 = vertices[indices[i + 2]].Position;

			Core::Ray localRay = {};
			localRay.Origin = localOrigin;
			localRay.Direction = localDirection;

			f32 distance;
			if (RayTriangleIntersection(localRay, v0, v1, v2, distance))
			{
				glm::vec3 localIntersection = localOrigin + localDirection * distance;
				glm::vec3 worldIntersection = glm::vec3(transform->GetModelMatrix() * glm::vec4(localIntersection, 1.0f));
				f32 worldDistance = glm::length(worldIntersection - start);

				if (worldDistance < closestDistance && worldDistance <= maxDistance)
				{
					closestDistance = worldDistance;
					closestObject = obj.get();
				}
			}
		}
	}

	Core::HitResult result = {};
	result.HitObject = closestObject;
	result.HitDistance = closestDistance;
	result.Hit = (closestObject != nullptr);

	return result;
}