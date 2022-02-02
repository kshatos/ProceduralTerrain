#include <Merlin/Core/core.hpp>
#include <Merlin/Scene/scene.hpp>
#include <Merlin/Render/render.hpp>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include <thread>
#include <fstream>
#include "cube_sphere.hpp"
#include "noise3d.hpp"
#include "erosion.hpp"
#include "custom_components.hpp"


using namespace Merlin;


class SceneLayer : public Layer
{
    std::shared_ptr<Camera> camera;
    std::shared_ptr<TransformComponent> camera_transform;
    std::shared_ptr<Texture2D> main_texture;
    std::shared_ptr<Material> main_material;
    std::shared_ptr<Cubemap> height_cubemap;
    std::shared_ptr<Cubemap> normal_cubemap;
    std::shared_ptr<FrameBuffer> fbuffer;
    std::shared_ptr<Mesh<Vertex_XNTBUV>> mesh;
    GameScene scene;

    float time_elapsed = 0.0f;
    float texture_scales[4]{ 2.5f, 2.5f, 2.5f, 2.5f };
    float water_scale = 0.01;
    float water_speed = 0.038f;
    float water_level = 0.543f;
    float water_depth_scale = 0.017f;
    glm::vec3 water_shallow_color{ 1.0f / 256.0, 5.0f / 256.0f, 38.0 / 256.0f };
    glm::vec3 water_deep_color{ 3.0f / 256.0, 29.0f / 256.0f, 156.0 / 256.0f };

public:
    void OnAttach() override
    {
        // Load Data
        main_texture = Texture2D::Create(
            ".\\Assets\\Textures\\debug.jpg",
            Texture2DProperties(
                TextureWrapMode::Repeat,
                TextureWrapMode::Repeat,
                TextureFilterMode::Linear));

        auto mountain_tex = Texture2D::Create(
            ".\\CustomAssets\\Textures\\HITW_Terrain-Textures-Set\\HITW-mntn-snow-greys.jpg",
            Texture2DProperties(
                TextureWrapMode::Repeat,
                TextureWrapMode::Repeat,
                TextureFilterMode::Linear));

        auto grass_tex = Texture2D::Create(
            ".\\CustomAssets\\Textures\\HITW_Terrain-Textures-Set\\HITW-TS2-range-shrub-grass-green.jpg",
            Texture2DProperties(
                TextureWrapMode::Repeat,
                TextureWrapMode::Repeat,
                TextureFilterMode::Linear));

        auto forest_tex = Texture2D::Create(
            ".\\CustomAssets\\Textures\\HITW_Terrain-Textures-Set\\HITW-TS2-forest-dark-grass-green.jpg",
            Texture2DProperties(
                TextureWrapMode::Repeat,
                TextureWrapMode::Repeat,
                TextureFilterMode::Linear));

        auto barren_tex = Texture2D::Create(
            ".\\CustomAssets\\Textures\\HITW_Terrain-Textures-Set\\HITW-TS2-range-shrub-red-desert.jpg",
            Texture2DProperties(
                TextureWrapMode::Repeat,
                TextureWrapMode::Repeat,
                TextureFilterMode::Linear));

        auto water_tex = Texture2D::Create(
            ".\\CustomAssets\\Textures\\Water0341normal.jpg",
            Texture2DProperties(
                TextureWrapMode::Repeat,
                TextureWrapMode::Repeat,
                TextureFilterMode::Linear));

        auto main_shader = Shader::CreateFromFiles(
            ".\\CustomAssets\\Shaders\\cube_sphere.vert",
            ".\\CustomAssets\\Shaders\\cube_sphere.frag");
        main_shader->Bind();
        main_material = std::make_shared<Material>(
            main_shader,
            BufferLayout
            {
                BufferElement{ShaderDataType::Float, "time" },
                BufferElement{ShaderDataType::Float, "u_water_level" },
                BufferElement{ShaderDataType::Float, "u_water_depth_scale" },
                BufferElement{ShaderDataType::Float3, "u_water_shallow_color" },
                BufferElement{ShaderDataType::Float3, "u_water_deep_color" },
                BufferElement{ShaderDataType::Float, "u_water_speed" },
                BufferElement{ShaderDataType::Float, "u_water_scale" },
                BufferElement{ShaderDataType::Float, "u_terrain_texture_scales[0]" },
                BufferElement{ShaderDataType::Float, "u_terrain_texture_scales[1]" },
                BufferElement{ShaderDataType::Float, "u_terrain_texture_scales[2]" },
                BufferElement{ShaderDataType::Float, "u_terrain_texture_scales[3]" }
            },
            std::vector<std::string>{
            "u_heightmap",
                "u_normal",
                "u_splatmap",
                "u_water_normalmap",
                "u_terrain_textures[0]",
                "u_terrain_textures[1]",
                "u_terrain_textures[2]",
                "u_terrain_textures[3]",
        }
        );
        main_material->SetTexture("u_water_normalmap", water_tex);
        main_material->SetTexture("u_terrain_textures[0]", mountain_tex);
        main_material->SetTexture("u_terrain_textures[1]", barren_tex);
        main_material->SetTexture("u_terrain_textures[2]", grass_tex);
        main_material->SetTexture("u_terrain_textures[3]", forest_tex);

        int resolution = 512;
        float spacing = 1.0f / resolution;
        auto heightmap_data = std::make_shared<CubemapData>(resolution, 1);
        auto normal_data = std::make_shared<CubemapData>(resolution, 3);
        auto splatmap_data = std::make_shared<CubemapData>(resolution, 4);

        // Calculate heightmap
        std::vector<std::thread> threads;
        for (int face_id = CubeFace::Begin; face_id < CubeFace::End; face_id++)
        {
            auto work = [face_id, heightmap_data]() {
                auto face = static_cast<CubeFace>(face_id);
                for (int j = 0; j < heightmap_data->GetResolution(); ++j)
                {
                    for (int i = 0; i < heightmap_data->GetResolution(); ++i)
                    {
                        auto point = CubemapData::CubePoint(heightmap_data->GetPixelCoordinates(face, i, j));
                        point = glm::normalize(point);

                        float ridge_noise = 1.00f * FractalRidgeNoise(point, 2.0f, 4, 0.5f, 2.0f);
                        float smooth_noise = 0.05f * FractalNoise(point, 4.0f, 4, 0.7f, 2.0f);
                        float blend = 0.5f * (glm::simplex(1.0f * point) + 1.0f);
                        float noise = blend * ridge_noise + (1.0 - blend) * smooth_noise;

                        heightmap_data->GetPixel(face, i, j, 0) = 0.5 + 0.1 * noise;
                    }
                }
            };
            threads.emplace_back(work);
        }
        for (auto& thread : threads)
            thread.join();

        // Erosion
        float grid_spacing = 1.0f / heightmap_data->GetResolution();
        ErosionParameters erosion_params;
        erosion_params.concentration_factor = 3.0f;
        erosion_params.erosion_time = 0.5f;
        erosion_params.evaporation_time = 1.0f;
        erosion_params.friction_time = 0.5;
        erosion_params.particle_start_volume = 0.8f * grid_spacing * grid_spacing;

        int n_particles = 1000;
        int n_steps = 10000;
        std::vector<ErosionParticle> particles(n_particles);
        for (auto& p : particles) { InitializeParticle(p, erosion_params); }
        for (int i = 0; i < n_steps; ++i)
            for (auto& p : particles)
                UpdateParticle(p, *heightmap_data, erosion_params);

        // Smooth to remove high frequency noise from erosion
        threads.clear();
        for (int face_id = CubeFace::Begin; face_id < CubeFace::End; face_id++)
        {
            auto work = [face_id, heightmap_data]() {
                auto face = static_cast<CubeFace>(face_id);
                for (int k = 0; k < 1; ++k)
                {
                    for (int j = 1; j < heightmap_data->GetResolution() - 1; ++j)
                    {
                        for (int i = 1; i < heightmap_data->GetResolution() - 1; ++i)
                        {
                            float average = 0.25f * (
                                heightmap_data->GetPixel(face, i + 1, j, 0) +
                                heightmap_data->GetPixel(face, i - 1, j, 0) +
                                heightmap_data->GetPixel(face, i, j + 1, 0) +
                                heightmap_data->GetPixel(face, i, j - 1, 0));
                            heightmap_data->GetPixel(face, i, j, 0) = average;
                        }
                    }
                }
            };
            threads.emplace_back(work);
        }
        for (auto& thread : threads)
            thread.join();

        // Calculate normal map
        threads.clear();
        for (int face_id = CubeFace::Begin; face_id < CubeFace::End; face_id++)
        {
            auto work = [face_id, heightmap_data, normal_data, spacing, resolution]() {
                auto face = static_cast<CubeFace>(face_id);
                for (int j = 0; j < heightmap_data->GetResolution(); ++j)
                {
                    for (int i = 0; i < heightmap_data->GetResolution(); ++i)
                    {
                        auto point = CubemapData::CubePoint(
                            heightmap_data->GetPixelCoordinates(face, i, j));
                        point = glm::normalize(point);

                        auto eu = SphereHeightmapUTangent(point, *heightmap_data);
                        auto ev = SphereHeightmapVTangent(point, *heightmap_data);

                        auto normal = -glm::cross(eu, ev);
                        normal = glm::normalize(normal);
                        normal = 0.5f * (normal + 1.0f);
                        normal_data->GetPixel(face, i, j, 0) = normal.x;
                        normal_data->GetPixel(face, i, j, 1) = normal.y;
                        normal_data->GetPixel(face, i, j, 2) = normal.z;
                    }
                }
            };
            threads.emplace_back(work);
        }
        for (auto& thread : threads)
            thread.join();

        // Calculate splatmap
        threads.clear();
        for (int face_id = CubeFace::Begin; face_id < CubeFace::End; face_id++)
        {
            auto work = [face_id, splatmap_data, heightmap_data]() {
                auto face = static_cast<CubeFace>(face_id);
                for (int j = 0; j < splatmap_data->GetResolution(); ++j)
                {
                    for (int i = 0; i < splatmap_data->GetResolution(); ++i)
                    {
                        auto point = CubemapData::CubePoint(splatmap_data->GetPixelCoordinates(face, i, j));
                        point = glm::normalize(point);

                        float cosT = glm::dot(point, glm::vec3(0.0, 1.0, 0.0));
                        float T = glm::acos(cosT);
                        float sin2T = glm::sin(2.0f * T);

                        float temperature = 1.0f - cosT * cosT;
                        temperature += 0.1f * SmoothNoise(5.0f * point + glm::vec3(0.0, 15.0, 0.0));
                        temperature = glm::clamp(temperature, 0.0f, 1.0f);


                        float rainfall = sin2T * sin2T;
                        rainfall += 0.4f * SmoothNoise(3.0f * point + glm::vec3(0.0, 15.0, 0.0));
                        rainfall = glm::clamp(rainfall, 0.0f, 1.0f);

                        float tundra = glm::clamp(0.5f - (temperature - 0.3f) / 0.1f, 0.0f, 1.0f);

                        float shrub = (1.0f - tundra);
                        float grass = (1.0f - tundra);
                        float forest = (1.0f - tundra);
                        if (rainfall < 0.1)
                        {
                            grass *= 0.0f;
                            forest *= 0.0f;
                        }
                        else if (rainfall < 0.3)
                        {
                            float blend = 0.5f - (rainfall - 0.2f) / (2.0f * 0.1f);
                            shrub *= blend;
                            grass *= (1.0 - blend);
                            forest *= 0.0;
                        }
                        else if (rainfall < 0.5)
                        {
                            shrub *= 0.0f;
                            forest *= 0.0f;
                        }
                        else if (rainfall < 0.7f)
                        {
                            float blend = 0.5f - (rainfall - 0.6f) / (2.0f * 0.1f);
                            shrub *= 0.0f;
                            grass *= blend;
                            forest *= (1.0 - blend);
                        }
                        else
                        {
                            shrub *= 0.0f;
                            grass *= 0.0f;
                        }

                        splatmap_data->GetPixel(face, i, j, 0) = tundra;
                        splatmap_data->GetPixel(face, i, j, 1) = shrub;
                        splatmap_data->GetPixel(face, i, j, 2) = grass;
                        splatmap_data->GetPixel(face, i, j, 3) = forest;
                    }
                }
            };
            threads.emplace_back(work);
        }
        for (auto& thread : threads)
            thread.join();

        height_cubemap = UploadCubemap(heightmap_data);
        normal_cubemap = UploadCubemap(normal_data);
        auto splatmap = UploadCubemap(splatmap_data);
        main_material->SetTexture("u_heightmap", height_cubemap);
        main_material->SetTexture("u_normal", normal_cubemap);
        main_material->SetTexture("u_splatmap", splatmap);

        mesh = BuildSphereMesh(20);
        //CalculateTangentFrame(mesh);
        auto varray = UploadMesh(mesh);

        {// Camera
            FrameBufferParameters fb_params;
            fb_params.width = 1000;
            fb_params.height = 1000;
            fb_params.color_buffer_format = ColorBufferFormat::RGBA8;
            fb_params.depth_buffer_format = DepthBufferFormat::DEPTH24_STENCIL8;
            fbuffer = FrameBuffer::Create(fb_params);

            camera = std::make_shared<PerspectiveCamera>(glm::pi<float>() / 3.0f, 1.0f, 0.1f, 20.0f);

            auto entity = scene.CreateEntity();
            auto camera_component = entity->AddComponent<CameraComponent>();
            auto transform_comp = entity->AddComponent<TransformComponent>();
            auto move_comp = entity->AddComponent<MovementControllerComponent>();
            camera_component->data.camera = camera;
            camera_component->data.frame_buffer = fbuffer;
            camera_component->data.clear_color = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
            transform_comp->transform.Translate(glm::vec3(0.0f, 0.0f, 5.0f));

            camera_transform = transform_comp;
        }
        {// Sphere
            auto entity = scene.CreateEntity();
            auto transform_comp = entity->AddComponent<TransformComponent>();
            auto mesh_render_comp = entity->AddComponent<MeshRenderComponent>();
            auto rotation_comp = entity->AddComponent<RotatingPlanetComponent>();
            rotation_comp->rotation_speed = 0.1;
            mesh_render_comp->data.vertex_array = varray;
            mesh_render_comp->data.material = main_material;
        }
        {// Light
            auto entity = scene.CreateEntity();
            auto light_comp = entity->AddComponent<DirectionalLightComponent>();
            light_comp->data.color = glm::vec3(1.0, 1.0, 1.0);
            light_comp->data.direction = glm::normalize(glm::vec3(1.0, 0.0, 0.0));
            light_comp->data.irradiance = 5.0f;
        }
        scene.OnAwake();
    }

    void OnDetatch() override
    {
    }

    void OnUpdate(float time_step) override
    {
        time_elapsed += time_step;
        main_material->SetUniformFloat("time", time_elapsed);

        scene.OnUpdate(time_step);
        {
            Renderer::SetViewport(
                0, 0,
                Application::Get().GeMaintWindow()->GetWidth(),
                Application::Get().GeMaintWindow()->GetHeight());
            Renderer::SetClearColor(glm::vec4(0.2f, 0.3f, 0.3f, 1.0f));
            Renderer::Clear();
            scene.RenderScene();
        }
        {
            auto& io = ImGui::GetIO();
            io.DisplaySize = ImVec2(
                (float)Application::Get().GeMaintWindow()->GetWidth(),
                (float)Application::Get().GeMaintWindow()->GetHeight());

            ImGui_ImplOpenGL3_NewFrame();
            ImGui::NewFrame();
            ImGui::Begin("Settings");

            ImGui::ColorPicker4("Shallow Water Color", &water_shallow_color.x);
            main_material->SetUniformFloat3("u_water_shallow_color", water_shallow_color);
            ImGui::ColorPicker4("Deep Water Color", &water_deep_color.x);
            main_material->SetUniformFloat3("u_water_deep_color", water_deep_color);
            ImGui::SliderFloat("u_water_level", &water_level, 0.0f, 1.0f);
            main_material->SetUniformFloat("u_water_level", water_level);
            ImGui::SliderFloat("u_water_depth_scale", &water_depth_scale, 0.0f, 0.1f);
            main_material->SetUniformFloat("u_water_depth_scale", water_depth_scale);
            ImGui::SliderFloat("Water Speed", &water_speed, 0.0f, 1.0f);
            main_material->SetUniformFloat("u_water_speed", water_speed);
            ImGui::SliderFloat("Water Scale", &water_scale, 0.0f, 1.0f);
            main_material->SetUniformFloat("u_water_scale", water_scale);
            ImGui::SliderFloat("Texture0 Scale", &texture_scales[0], 0.001f, 10.0f);
            main_material->SetUniformFloat("u_terrain_texture_scales[0]", texture_scales[0]);
            ImGui::SliderFloat("Texture1 Scale", &texture_scales[1], 0.001f, 10.0f);
            main_material->SetUniformFloat("u_terrain_texture_scales[1]", texture_scales[1]);
            ImGui::SliderFloat("Texture2 Scale", &texture_scales[2], 0.001f, 10.0f);
            main_material->SetUniformFloat("u_terrain_texture_scales[2]", texture_scales[2]);
            ImGui::SliderFloat("Texture3 Scale", &texture_scales[3], 0.001f, 10.0f);
            main_material->SetUniformFloat("u_terrain_texture_scales[3]", texture_scales[3]);
            ImGui::End();

            auto s_buffer = fbuffer;
            auto s_buffer_params = s_buffer->GetParameters();
            uint32_t tex_id = s_buffer->GetColorAttachmentID();
            ImGui::Image((ImTextureID)tex_id, ImVec2{ (float)s_buffer_params.width, (float)s_buffer_params.height });

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
    }

    virtual void HandleEvent(AppEvent& app_event) override
    {
    }

};

class ProceduralTerrainApp : public Application
{
public:
    ProceduralTerrainApp()
    {
        PushLayerBack(std::make_shared<SceneLayer>());
    }
};


void main()
{
    ProceduralTerrainApp app;
    app.Run();
}
