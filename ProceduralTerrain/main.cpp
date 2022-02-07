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
#include "editor_window.hpp"
#include "terrain.hpp"

using namespace Merlin;


class SceneLayer : public Layer
{
    std::shared_ptr<Material> terrain_material = nullptr;

    std::shared_ptr<CubemapData> height_data = nullptr;
    std::shared_ptr<CubemapData> normal_data = nullptr;
    std::shared_ptr<CubemapData> splat_data = nullptr;

    std::shared_ptr<Cubemap> height_cubemap = nullptr;
    std::shared_ptr<Cubemap> normal_cubemap = nullptr;
    std::shared_ptr<Cubemap> splat_cubemap = nullptr;

    std::shared_ptr<EditorWindow> editor_window = nullptr;

    std::shared_ptr<FrameBuffer> fbuffer;
    GameScene scene;

    float time_elapsed = 0.0f;

public:

    void LoadResources()
    {
        // Load textures
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

        // Compile shader program
        auto main_shader = Shader::CreateFromFiles(
            ".\\CustomAssets\\Shaders\\cube_sphere.vert",
            ".\\CustomAssets\\Shaders\\cube_sphere.frag");
        main_shader->Bind();

        // Specify material and set textures
        terrain_material = std::make_shared<Material>(
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
        terrain_material->SetTexture("u_water_normalmap", water_tex);
        terrain_material->SetTexture("u_terrain_textures[0]", mountain_tex);
        terrain_material->SetTexture("u_terrain_textures[1]", barren_tex);
        terrain_material->SetTexture("u_terrain_textures[2]", grass_tex);
        terrain_material->SetTexture("u_terrain_textures[3]", forest_tex);
    }

    void InitializeCubemaps()
    {
        int resolution = 512;

        height_data = std::make_shared<CubemapData>(resolution, 1);
        normal_data = std::make_shared<CubemapData>(resolution, 3);
        splat_data = std::make_shared<CubemapData>(resolution, 4);

        height_cubemap = Cubemap::Create(resolution, 1);
        normal_cubemap = Cubemap::Create(resolution, 3);
        splat_cubemap = Cubemap::Create(resolution, 4);

        terrain_material->SetTexture("u_heightmap", height_cubemap);
        terrain_material->SetTexture("u_normal", normal_cubemap);
        terrain_material->SetTexture("u_splatmap", splat_cubemap);
    }

    void BuildScene()
    {
        {// Camera
            FrameBufferParameters fb_params;
            fb_params.width = 900;
            fb_params.height = 900;
            fb_params.color_buffer_format = ColorBufferFormat::RGBA8;
            fb_params.depth_buffer_format = DepthBufferFormat::DEPTH24_STENCIL8;
            fbuffer = FrameBuffer::Create(fb_params);

            auto entity = scene.CreateEntity();
            auto camera_component = entity->AddComponent<CameraComponent>();
            auto transform_comp = entity->AddComponent<TransformComponent>();
            auto move_comp = entity->AddComponent<MovementControllerComponent>();
            camera_component->data.camera = std::make_shared<PerspectiveCamera>(
                glm::pi<float>() / 3.0f, 1.0f, 0.1f, 20.0f);
            camera_component->data.frame_buffer = fbuffer;
            camera_component->data.clear_color = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
            transform_comp->transform.Translate(glm::vec3(0.0f, 0.0f, 5.0f));
        }
        {// Sphere
            auto mesh = BuildSphereMesh(20);
            auto varray = UploadMesh(mesh);

            auto entity = scene.CreateEntity();
            auto transform_comp = entity->AddComponent<TransformComponent>();
            auto mesh_render_comp = entity->AddComponent<MeshRenderComponent>();
            auto rotation_comp = entity->AddComponent<RotatingPlanetComponent>();
            rotation_comp->rotation_speed = 0.1;
            mesh_render_comp->data.vertex_array = varray;
            mesh_render_comp->data.material = terrain_material;
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

    void CalculateMaps()
    {
        // Procedurally generate map data
        GenerateNoiseHeightmap(height_data);
        //ErodeHeightmap(height_data);
        //SmoothMap(height_data, 1);
        CalculateNormalMap(height_data, normal_data);
        GenerateBiomes(splat_data);

        // Upload data to textures
        for (int face_id = CubeFace::Begin; face_id < CubeFace::End; face_id++)
        {
            auto face = static_cast<CubeFace>(face_id);

            height_cubemap->SetFaceData(face, height_data->GetFaceDataPointer(face));
            normal_cubemap->SetFaceData(face, normal_data->GetFaceDataPointer(face));
            splat_cubemap->SetFaceData(face, splat_data->GetFaceDataPointer(face));
        }
    }

    void OnAttach() override
    {
        LoadResources();
        InitializeCubemaps();
        BuildScene();
        CalculateMaps();
        editor_window = std::make_shared<EditorWindow>(terrain_material);
    }

    void OnUpdate(float time_step) override
    {
        time_elapsed += time_step;
        terrain_material->SetUniformFloat("time", time_elapsed);

        scene.OnUpdate(time_step);
        scene.RenderScene();

        {
            Renderer::SetViewport(
                0, 0,
                Application::Get().GeMaintWindow()->GetWidth(),
                Application::Get().GeMaintWindow()->GetHeight());
            Renderer::SetClearColor(glm::vec4(0.2f, 0.3f, 0.3f, 1.0f));
            Renderer::Clear();
        }
        {
            auto& io = ImGui::GetIO();
            io.DisplaySize = ImVec2(
                (float)Application::Get().GeMaintWindow()->GetWidth(),
                (float)Application::Get().GeMaintWindow()->GetHeight());

            ImGui_ImplOpenGL3_NewFrame();
            ImGui::NewFrame();

            editor_window->Draw(fbuffer);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
    }

    void OnDetatch() override {};

    void HandleEvent(AppEvent& app_event) override {}

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
