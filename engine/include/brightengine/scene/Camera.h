#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace brightengine
{
    class Camera
    {
    public:
        Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up,
               float fovDegrees, float aspectRatio, float nearPlane, float farPlane)
            : m_position(position)
            , m_target(target)
            , m_up(up)
            , m_fovDegrees(fovDegrees)
            , m_aspectRatio(aspectRatio)
            , m_nearPlane(nearPlane)
            , m_farPlane(farPlane)
        {
        }

        glm::mat4 GetViewMatrix() const
        {
            return glm::lookAt(m_position, m_target, m_up);
        }

        glm::mat4 GetProjectionMatrix() const
        {
            return glm::perspective(glm::radians(m_fovDegrees), m_aspectRatio, m_nearPlane, m_farPlane);
        }

        // Translate along the current view direction -- position and target
        // move together, so the camera keeps looking the same way. No
        // rotation/mouse-look yet, that's a separate piece.
        void MoveForward(float amount)
        {
            glm::vec3 forward = glm::normalize(m_target - m_position);
            m_position += forward * amount;
            m_target += forward * amount;
        }

        void MoveRight(float amount)
        {
            glm::vec3 forward = glm::normalize(m_target - m_position);
            glm::vec3 right = glm::normalize(glm::cross(forward, m_up));
            m_position += right * amount;
            m_target += right * amount;
        }

    private:
        glm::vec3 m_position;
        glm::vec3 m_target;
        glm::vec3 m_up;
        float m_fovDegrees;
        float m_aspectRatio;
        float m_nearPlane;
        float m_farPlane;
    };
}
