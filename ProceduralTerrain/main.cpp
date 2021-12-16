#include <Merlin/Core/core.hpp>
#include <Merlin/Scene/scene.hpp>
#include <Merlin/Render/render.hpp>
#include "cube_sphere.hpp"

using namespace Merlin;


class SceneLayer : public Layer
{
    std::shared_ptr<Camera> camera;
    std::shared_ptr<Shader> main_shader;
    std::shared_ptr<Texture2D> main_texture;
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

        main_shader = Shader::CreateFromFiles(
            ".\\CoreAssets\\Shaders\\basic_lit.vert",
            ".\\CoreAssets\\Shaders\\basic_lit.frag");
        main_shader->Bind();
        main_shader->SetUniformInt("u_Texture", 0);

        mesh = BuildSphereMesh(20);
        auto varray = UploadMesh(*mesh);

        // Camera
        camera = std::make_shared<PerspectiveCamera>(glm::pi<float>() / 2.0f, 1.0f, 0.5f, 20.0f);
        camera->GetTransform().Translate(glm::vec3(0.0f, 0.0f, 5.0f));
        scene.SetCamera(camera);

        {// Sphere
            auto entity = std::make_shared<Entity>();
            auto transform_comp = entity->AddComponent<TransformComponent>();
            auto mesh_render_comp = entity->AddComponent<MeshRenderComponent>();
            mesh_render_comp->varray = varray;
            mesh_render_comp->shader = main_shader;
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
            main_texture->Bind();
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

        camera->GetTransform().Rotate(up, Input::GetMouseScrollDelta().y * 1.0e-1f);
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
