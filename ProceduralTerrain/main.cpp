#include <Merlin/Core/core.hpp>
#include <Merlin/Scene/scene.hpp>
#include <Merlin/Render/render.hpp>
#include <thread>
#include "cube_sphere.hpp"
#include "noise3d.hpp"

using namespace Merlin;


class SceneLayer : public Layer
{
    std::shared_ptr<Camera> camera;
    std::shared_ptr<Texture2D> main_texture;
    std::shared_ptr<Material> main_material;
    std::shared_ptr<Cubemap> main_cubemap;
    std::shared_ptr<Mesh<Vertex_XNUV>> mesh;
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
        main_shader->SetUniformInt("u_albedo", 0);
        main_material = std::make_shared<Material>(
            main_shader,
            BufferLayout{},
            std::vector<std::string>{});

        auto cubemap_data = std::make_shared<CubemapData>(512, 1);
        std::vector<std::thread> threads;
        for (int face_id = CubeFace::Begin; face_id < CubeFace::End; face_id++)
        {
            auto work = [face_id, cubemap_data]() {
                auto face = static_cast<CubeFace>(face_id);
                for (int j = 0; j < cubemap_data->GetResolution(); ++j)
                {
                    for (int i = 0; i < cubemap_data->GetResolution(); ++i)
                    {
                        auto point = cubemap_data->GetPixelCubePoint(face, i, j);
                        point = glm::normalize(point);

                        float ridge_noise = FractalRidgeNoise(point, 10.0f, 4, 0.7f, 2.0f);
                        float smooth_noise = FractalNoise(point, 5.0f, 4, 0.7f, 2.0f);
                        float blend = 0.5f * (glm::simplex(3.0f * point) + 1.0f);
                        float noise = blend * ridge_noise + (1.0 - blend) * smooth_noise;

                        cubemap_data->GetPixel(face, i, j, 0) = 0.5 + 0.2 * noise;
                    }
                }
            };
            threads.emplace_back(work);
        }
        for (auto& thread : threads)
            thread.join();

        main_cubemap = UploadCubemap(cubemap_data);

        mesh = BuildSphereMesh(20);
        auto varray = UploadMesh(mesh);

        // Camera
        camera = std::make_shared<PerspectiveCamera>(glm::pi<float>() / 3.0f, 1.0f, 0.1f, 20.0f);
        camera->GetTransform().Translate(glm::vec3(0.0f, 0.0f, 5.0f));
        scene.SetCamera(camera);

        {// Sphere
            auto entity = std::make_shared<Entity>();
            auto transform_comp = entity->AddComponent<TransformComponent>();
            auto mesh_render_comp = entity->AddComponent<MeshRenderComponent>();
            mesh_render_comp->varray = varray;
            mesh_render_comp->material = main_material;
            scene.AddEntity(entity);
        }
        {// Light
            auto entity = std::make_shared<Entity>();
            auto light_comp = entity->AddComponent<DirectionalLightComponent>();
            light_comp->data.color = glm::vec3(0.5, 0.7, 0.5);
            light_comp->data.direction = glm::normalize(glm::vec3(1.0, 0.0, -1.0));
            scene.AddEntity(entity);
        }

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
            main_cubemap->Bind();
            scene.RenderScene();
        }
    }

    virtual void HandleEvent(AppEvent& app_event) override
    {
    }

    void MoveCamera(float time_step)
    {
        const auto& up = camera->GetTransform().Up();
        const auto& right = camera->GetTransform().Right();
        const auto& forward = camera->GetTransform().Forward();
        float speed = 5.0e-1f;
        if (Input::GetKeyDown(Key::W))
            camera->GetTransform().Translate(+forward * speed * time_step);
        if (Input::GetKeyDown(Key::A))
            camera->GetTransform().Translate(-right* speed * time_step);
        if (Input::GetKeyDown(Key::S))
            camera->GetTransform().Translate(-forward * speed * time_step);
        if (Input::GetKeyDown(Key::D))
            camera->GetTransform().Translate(+right * speed * time_step);
        if (Input::GetKeyDown(Key::Z))
            camera->GetTransform().Translate(+up * speed * time_step);
        if (Input::GetKeyDown(Key::X))
            camera->GetTransform().Translate(-up * speed * time_step);

        camera->GetTransform().Rotate(up, Input::GetMouseScrollDelta().y * time_step * 20.0);
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
