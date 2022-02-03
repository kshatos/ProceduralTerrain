#ifndef EDITOR_WINDOW_HPP
#define EDITOR_WINDOW_HPP
#include <imgui.h>
#include <Merlin/Render/material.hpp>


class EditorWindow
{
    float texture_scales[4]{ 2.5f, 2.5f, 2.5f, 2.5f };
    float water_scale = 0.01;
    float water_speed = 0.038f;
    float water_level = 0.543f;
    float water_depth_scale = 0.017f;
    glm::vec3 water_shallow_color{ 1.0f / 256.0, 5.0f / 256.0f, 38.0 / 256.0f };
    glm::vec3 water_deep_color{ 3.0f / 256.0, 29.0f / 256.0f, 156.0 / 256.0f };

    std::shared_ptr<Material> m_material = nullptr;

public:
    EditorWindow(const std::shared_ptr<Material>& material) :
        m_material(material)
    {
    }


    void Draw()
    {
        ImGui::Begin(
            "Settings",
            false,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_MenuBar);
        ImGui::SetWindowSize(ImVec2(400, 10000));

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

        ImGui::End();

        SetMaterialProperties();
    }

private:
    void DrawWaterTab()
    {
        ImGui::ColorPicker4("Shallow Water Color", &water_shallow_color.x);
        ImGui::ColorPicker4("Deep Water Color", &water_deep_color.x);
        ImGui::SliderFloat("u_water_level", &water_level, 0.0f, 1.0f);
        ImGui::SliderFloat("u_water_depth_scale", &water_depth_scale, 0.0f, 0.1f);
        ImGui::SliderFloat("Water Speed", &water_speed, 0.0f, 1.0f);
        ImGui::SliderFloat("Water Scale", &water_scale, 0.0f, 1.0f);
    }

    void DrawTerrainTab()
    {
        ImGui::SliderFloat("Texture0 Scale", &texture_scales[0], 0.001f, 10.0f);
        ImGui::SliderFloat("Texture1 Scale", &texture_scales[1], 0.001f, 10.0f);
        ImGui::SliderFloat("Texture2 Scale", &texture_scales[2], 0.001f, 10.0f);
        ImGui::SliderFloat("Texture3 Scale", &texture_scales[3], 0.001f, 10.0f);
    }

    void SetMaterialProperties()
    {
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
