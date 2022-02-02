#ifndef CUSTOM_COMPONENTS_HPP
#define CUSTOM_COMPONENTS_HPP
#include <Merlin/Scene/component.hpp>
#include <Merlin/Scene/core_components.hpp>
#include <memory>


class RotatingPlanetComponent : public Merlin::Component
{
    std::shared_ptr<Merlin::TransformComponent> m_transform = nullptr;
public:
    glm::vec3 axis{ 0.0f, 1.0f, 0.0f };
    float rotation_speed = 1.0f;

    RotatingPlanetComponent(Merlin::Entity* parent) : Merlin::Component(parent)
    {
    }

    void OnAwake() override
    {
        m_transform = m_parent->GetComponent<Merlin::TransformComponent>();
    }

    void OnUpdate(float time_step)
    {
        m_transform->transform.Rotate(
            axis, time_step * rotation_speed);
    }
};


class MovementControllerComponent : public Merlin::Component
{
    std::shared_ptr<Merlin::TransformComponent> m_transform = nullptr;
public:
    float movement_speed = 1.0f;
    float rotation_speed = 1.0f;

    MovementControllerComponent(Merlin::Entity* parent) : Merlin::Component(parent)
    {
    }

    void OnAwake() override
    {
        m_transform = m_parent->GetComponent<Merlin::TransformComponent>();
    }

    void OnUpdate(float time_step)
    {
        auto& transform = m_transform->transform;

        const auto& up = transform.Up();
        const auto& right = transform.Right();
        const auto& forward = transform.Forward();

        if (Input::GetKeyDown(Key::W))
            transform.Translate(+forward * movement_speed * time_step);
        if (Input::GetKeyDown(Key::A))
            transform.Translate(-right * movement_speed * time_step);
        if (Input::GetKeyDown(Key::S))
            transform.Translate(-forward * movement_speed * time_step);
        if (Input::GetKeyDown(Key::D))
            transform.Translate(+right * movement_speed * time_step);
        if (Input::GetKeyDown(Key::Z))
            transform.Translate(+up * movement_speed * time_step);
        if (Input::GetKeyDown(Key::X))
            transform.Translate(-up * movement_speed * time_step);

        if (Input::GetKeyDown(Key::Q))
            transform.Rotate(up, rotation_speed * time_step);
        if (Input::GetKeyDown(Key::E))
            transform.Rotate(up, -rotation_speed * time_step);
    }
};

#endif