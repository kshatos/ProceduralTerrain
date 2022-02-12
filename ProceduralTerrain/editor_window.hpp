#ifndef EDITOR_WINDOW_HPP
#define EDITOR_WINDOW_HPP
#include <imgui.h>
#include <Merlin/Render/material.hpp>
#include <Merlin/Core/logger.hpp>
#include <functional>


class EditorWindow
{
    const float settings_width = 300.0f;
    const float element_width = 150.0f;
    float texture_scales[4]{ 2.5f, 2.5f, 2.5f, 2.5f };
    float water_scale = 0.08;
    float water_speed = 0.1f;
    float water_level = 0.5f;
    float water_depth_scale = 0.017f;
    glm::vec3 water_shallow_color{ 0.0f / 256.0, 64.0f / 256.0f, 89.0 / 256.0f };
    glm::vec3 water_deep_color{ 0.0f / 256.0, 28.0f / 256.0f, 34.0 / 256.0f };

    ImVec2 viewport_size{ 0.0f, 0.0f };

    std::shared_ptr<Material> m_material = nullptr;

public:
    const ImVec2& GetViewportSize() { return viewport_size; }

    EditorWindow(const std::shared_ptr<Material>& material) :
        m_material(material)
    {
    }

    void Draw(const CameraRenderData& camera_data)
    {
        auto& fbuffer = camera_data.frame_buffer;
        auto& display_size = ImGui::GetIO().DisplaySize;
        auto& fbuffer_params = fbuffer->GetParameters();
        uint32_t tex_id = fbuffer->GetColorAttachmentID();
        ImVec2 fbuffer_size{
                (float)fbuffer_params.width,
                (float)fbuffer_params.height };

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::Begin(
            "Settings",
            false,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar);
        ImGui::SetWindowSize(display_size);

        // Settings Tabs
        ImGui::BeginChild("Settings Group", ImVec2(settings_width, 0), true);
        if (ImGui::BeginTabBar("Settings Groups"))
        {
            if (ImGui::BeginTabItem("Water"))
            {
                DrawWaterTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Terrain"))
            {
                DrawTerrainTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
        ImGui::EndChild();

        // Viewport
        ImGui::SameLine();
        ImGui::BeginChild(
            "Viewport",
            ImVec2(0, 0),
            true,
            ImGuiWindowFlags_NoScrollbar);
        ImGui::Image((ImTextureID)tex_id, fbuffer_size);
        auto new_size = ImGui::GetWindowSize();
        //auto new_size = ImGui::GetContentRegionAvail();

        if (new_size.x != viewport_size.x || new_size.y != viewport_size.y)
        {
            fbuffer_params.width = new_size.x;
            fbuffer_params.height = new_size.y;
            fbuffer->Rebuild();
            float aspect_ratio = new_size.x / new_size.y;
            camera_data.camera->SetAspectRatio(aspect_ratio);
            viewport_size = new_size;
        }

        ImGui::EndChild();

        ImGui::End();
        SetMaterialProperties();
    }

private:
    void DrawWaterTab()
    {
        ImGui::Separator();
        ImGui::SetNextItemWidth(element_width);
        ImGui::ColorPicker4("Shallow Water Color", &water_shallow_color.x);

        ImGui::Separator();
        ImGui::SetNextItemWidth(element_width);
        ImGui::ColorPicker4("Deep Water Color", &water_deep_color.x);

        ImGui::Separator();
        ImGui::SetNextItemWidth(element_width);
        ImGui::SliderFloat("Water Level", &water_level, 0.3f, 0.8f);

        ImGui::Separator();
        ImGui::SetNextItemWidth(element_width);
        ImGui::SliderFloat("Water Depth Scale", &water_depth_scale, 0.0f, 0.05f);

        ImGui::Separator();
        ImGui::SetNextItemWidth(element_width);
        ImGui::SliderFloat("Water Wave Speed", &water_speed, 0.0f, 0.3f);

        ImGui::Separator();
        ImGui::SetNextItemWidth(element_width);
        ImGui::SliderFloat("Water Texture Scale", &water_scale, 0.01f, 0.5f);

        ImGui::Separator();
    }

    void DrawTerrainTab()
    {
        ImGui::Separator();
        ImGui::SetNextItemWidth(element_width);
        ImGui::SliderFloat("Texture0 Scale", &texture_scales[0], 0.001f, 10.0f);

        ImGui::Separator();
        ImGui::SetNextItemWidth(element_width);
        ImGui::SliderFloat("Texture1 Scale", &texture_scales[1], 0.001f, 10.0f);

        ImGui::Separator();
        ImGui::SetNextItemWidth(element_width);
        ImGui::SliderFloat("Texture2 Scale", &texture_scales[2], 0.001f, 10.0f);

        ImGui::Separator();
        ImGui::SetNextItemWidth(element_width);
        ImGui::SliderFloat("Texture3 Scale", &texture_scales[3], 0.001f, 10.0f);

        ImGui::Separator();
    }

    void SetMaterialProperties()
    {
        if (m_material == nullptr)
            return;

        m_material->SetUniformFloat3("u_water_shallow_color", water_shallow_color);
        m_material->SetUniformFloat3("u_water_deep_color", water_deep_color);
        m_material->SetUniformFloat("u_water_level", water_level);
        m_material->SetUniformFloat("u_water_depth_scale", water_depth_scale);
        m_material->SetUniformFloat("u_water_speed", water_speed);
        m_material->SetUniformFloat("u_water_scale", water_scale);

        m_material->SetUniformFloat("u_terrain_texture_scales[0]", texture_scales[0]);
        m_material->SetUniformFloat("u_terrain_texture_scales[1]", texture_scales[1]);
        m_material->SetUniformFloat("u_terrain_texture_scales[2]", texture_scales[2]);
        m_material->SetUniformFloat("u_terrain_texture_scales[3]", texture_scales[3]);
    }

};

#endif
