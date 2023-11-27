#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Random.h"
#include "Walnut/Timer.h"
#include "Renderer.h"
#include "Camera.h"

#include "glm/gtc/type_ptr.hpp"
#include "Walnut/Input/Input.h"

using namespace Walnut;

class ExampleLayer : public Walnut::Layer
{
public:
	ExampleLayer()
		: m_Camera(45.0f, 0.1f, 100.0f)
	{
		Material& pinkSphere = m_Scene.Materials.emplace_back();
		pinkSphere.Albedo = { 1.0f, 0.0f, 1.0f };
		pinkSphere.Roughness = 0.0f;
		Material& blueSphere = m_Scene.Materials.emplace_back();
		blueSphere.Albedo = { 0.2f, 0.3f, 1.0f };
		blueSphere.Roughness = 0.1f;
		Material& orangeSphere = m_Scene.Materials.emplace_back();
		orangeSphere.Albedo = { 0.8f, 0.5f, 0.2f };
		orangeSphere.Roughness = 0.1f;
		orangeSphere.EmissionColor = orangeSphere.Albedo;
		orangeSphere.EmissionPower = 2.0f;

		{
			Sphere sphere;
			sphere.Position = { 0.0f, 0.0f, 0.0f };
			sphere.Radius = 1.0f;
			sphere.MaterialIndex = 0;
			m_Scene.Spheres.push_back(sphere);
		}
		{
			Sphere sphere;
			sphere.Position = { 2.0f, 0.0f, 0.0f };
			sphere.Radius = 1.0f;
			sphere.MaterialIndex = 2;
			m_Scene.Spheres.push_back(sphere);
		}
		{
			Sphere sphere;
			sphere.Position = { 0.0f, -101.0f, 0.0f };
			sphere.Radius = 100.0f;
			sphere.MaterialIndex = 1;
			m_Scene.Spheres.push_back(sphere);
		}
	}

	virtual void OnUpdate(float ts) override
	{
		if (Input::IsKeyDown(KeyCode::Escape))
		{
			exit(0);
		}
		if (m_Camera.OnUpdate(ts))
		{
			m_Renderer.ResetFrameIndex();
		}
	}

	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");
		ImGui::Text("Last render: %.3fms", m_LastRenderTime);
		ImGui::Spacing();
		ImGui::Checkbox("Accumulate", &m_Renderer.GetSettings().Accumulate);
		ImGui::Checkbox("Slow Random", &m_Renderer.GetSettings().SlowRandom);
		if (ImGui::Button("Reset"))
		{
			m_Renderer.ResetFrameIndex();
		}
		ImGui::End();

		ImGui::Begin("Scene");
		if (ImGui::CollapsingHeader("Sphere"))
		{
			for (size_t i = 0; i < m_Scene.Spheres.size(); ++i)
			{
				// 提供唯一ID, 避免for循环中创建的Label一样的UI同步更改
				ImGui::PushID((int)i);
				if (ImGui::CollapsingHeader((("Sphere ") + std::to_string(i)).c_str()))
				{
					Sphere& sphere = m_Scene.Spheres[i];
					// ImGui 使用 Label 唯一标识
					ImGui::DragFloat3("Position", glm::value_ptr(sphere.Position), 0.1f);
					ImGui::DragFloat("Radius", &sphere.Radius, 0.1f);
					ImGui::DragInt("Material", &sphere.MaterialIndex, 1, 0, (int)m_Scene.Materials.size() - 1);
					ImGui::Separator();
				}
				ImGui::PopID();
			}
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		if (ImGui::CollapsingHeader("Material"))
		{
			for (size_t i = 0; i < m_Scene.Materials.size(); ++i)
			{
				// 提供唯一ID, 避免for循环中创建的Label一样的UI同步更改
				ImGui::PushID((int)i);
				if (ImGui::CollapsingHeader((("Material ") + std::to_string(i)).c_str()))
				{
					Material& material = m_Scene.Materials[i];
					// ImGui 使用 Label 唯一标识
					ImGui::ColorEdit3("Albedo", glm::value_ptr(material.Albedo));
					ImGui::DragFloat("Roughness", &material.Roughness, 0.05f, 0.0f, 1.0f);
					ImGui::DragFloat("Metallic", &material.Metallic, 0.05f, 0.0f, 1.0f);
					ImGui::ColorEdit3("Emission Color", glm::value_ptr(material.EmissionColor));
					ImGui::DragFloat("Emission Power", &material.EmissionPower, 0.05f, 0.0f, FLT_MAX);
					ImGui::Separator();
				}
				ImGui::PopID();
			}
		}
		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");
		m_ViewportWidth = (uint32_t)ImGui::GetContentRegionAvail().x;
		m_ViewportHeight = (uint32_t)ImGui::GetContentRegionAvail().y;

		auto image = m_Renderer.GetFinalImage();
		if (image)
			ImGui::Image(image->GetDescriptorSet(), { (float)image->GetWidth(), (float)image->GetHeight() }, ImVec2(0, 1), ImVec2(1, 0));
		ImGui::End();
		ImGui::PopStyleVar();

		Render();

		//ImGui::ShowDemoWindow();
	}

	void Render()
	{
		Timer timer;


		// renderer resize
		m_Renderer.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_Camera.OnResize(m_ViewportWidth, m_ViewportHeight);

		// renderer render
		m_Renderer.Render(m_Scene, m_Camera);


		m_LastRenderTime = timer.ElapsedMillis();
	}




private:
	Renderer m_Renderer;
	Camera m_Camera;
	Scene m_Scene;
	uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
	float m_LastRenderTime = 0.0f;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Ray Tracing";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ExampleLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});
	return app;
}