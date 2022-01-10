#include <Merlin/Core/core.hpp>
#include <Merlin/Scene/scene.hpp>
#include <Merlin/Render/render.hpp>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include <thread>
#include "cube_sphere.hpp"
#include "noise3d.hpp"

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

public:
    void OnAttach() override
    {
        // Load Data
        main_texture = Texture2D::Create(
            ".\\CoreAssets\\Textures\\debug.jpg",
            Texture2DProperties(
                TextureWrapMode::Repeat,
                TextureWrapMode::Repeat,
                TextureFilterMode::Linear));

        auto main_shader = Shader::CreateFromFiles(
            ".\\Assets\\Shaders\\cube_sphere.vert",
            ".\\Assets\\Shaders\\cube_sphere.frag");
        main_shader->Bind();
        main_material = std::make_shared<Material>(
            main_shader,
            BufferLayout{},
            std::vector<std::string>{
                "u_albedo",
                "u_normal"
            }
        );

        int resolution = 512;
        float spacing = 1.0f / resolution;
        auto heightmap_data = std::make_shared<CubemapData>(resolution, 1);
        auto normal_data = std::make_shared<CubemapData>(resolution, 3);
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
                        auto point = heightmap_data->GetPixelCubePoint(face, i, j);
                        point = glm::normalize(point);

                        float ridge_noise = FractalRidgeNoise(point, 4.0f, 4, 0.7f, 2.0f);
                        float smooth_noise = FractalNoise(point, 1.0f, 4, 0.7f, 2.0f);
                        float blend = 0.5f * (glm::simplex(3.0f * point) + 1.0f);
                        float noise = blend * ridge_noise + (1.0 - blend) * smooth_noise;

                        heightmap_data->GetPixel(face, i, j, 0) = 0.5 + 0.01 * noise;
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
                        uint32_t ie = glm::min(i + 1, resolution - 1);
                        uint32_t iw = glm::max(i - 1, 0);
                        uint32_t jn = glm::min(j + 1, resolution - 1);
                        uint32_t js = glm::max(j - 1, 0);

                        auto pe = glm::normalize(heightmap_data->GetPixelCubePoint(face, ie, j));
                        pe *= 1.0 + heightmap_data->GetPixel(face, ie, j, 0);

                        auto pw = glm::normalize(heightmap_data->GetPixelCubePoint(face, iw, j));
                        pw *= 1.0 + heightmap_data->GetPixel(face, iw, j, 0);

                        auto pn = glm::normalize(heightmap_data->GetPixelCubePoint(face, i, jn));
                        pn *= 1.0 + heightmap_data->GetPixel(face, i, jn, 0);

                        auto ps = glm::normalize(heightmap_data->GetPixelCubePoint(face, i, js));
                        ps *= 1.0 + heightmap_data->GetPixel(face, i, js, 0);

                        auto normal = glm::cross(pe - pw, pn - ps);
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

        height_cubemap = UploadCubemap(heightmap_data);
        normal_cubemap = UploadCubemap(normal_data);
        main_material->SetTexture("u_albedo", height_cubemap);
        main_material->SetTexture("u_normal", normal_cubemap);

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
            mesh_render_comp->data.vertex_array = varray;
            mesh_render_comp->data.material = main_material;
        }
        {// Light
            auto entity = scene.CreateEntity();
            auto light_comp = entity->AddComponent<DirectionalLightComponent>();
            light_comp->data.color = glm::vec3(0.5, 0.7, 0.5);
            light_comp->data.direction = glm::normalize(glm::vec3(0.0, -1.0, 0.0));
            light_comp->data.irradiance = 10.0f;
        }
        scene.OnAwake();
    }

    void OnDetatch() override
    {
    }

    void OnUpdate(float time_step) override
    {
        MoveCamera(time_step);
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

    void MoveCamera(float time_step)
    {
        const auto& up = camera_transform->transform.Up();
        const auto& right = camera_transform->transform.Right();
        const auto& forward = camera_transform->transform.Forward();
        float speed = 5.0e-1f;
        if (Input::GetKeyDown(Key::W))
            camera_transform->transform.Translate(+forward * speed * time_step);
        if (Input::GetKeyDown(Key::A))
            camera_transform->transform.Translate(-right * speed * time_step);
        if (Input::GetKeyDown(Key::S))
            camera_transform->transform.Translate(-forward * speed * time_step);
        if (Input::GetKeyDown(Key::D))
            camera_transform->transform.Translate(+right * speed * time_step);
        if (Input::GetKeyDown(Key::Z))
            camera_transform->transform.Translate(+up * speed * time_step);
        if (Input::GetKeyDown(Key::X))
            camera_transform->transform.Translate(-up * speed * time_step);

        camera_transform->transform.Rotate(up, Input::GetMouseScrollDelta().y * time_step * 20.0);
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
