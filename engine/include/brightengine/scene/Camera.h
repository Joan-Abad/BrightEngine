#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace brightengine
{
    class Camera
    {
    public:
        Camera(glm::vec3 position, float yawDegrees, float pitchDegrees, glm::vec3 up,
               float fovDegrees, float aspectRatio, float nearPlane, float farPlane)
            : m_position(position)
            , m_yaw(yawDegrees)
            , m_pitch(pitchDegrees)
            , m_up(up)
            , m_fovDegrees(fovDegrees)
            , m_aspectRatio(aspectRatio)
            , m_nearPlane(nearPlane)
            , m_farPlane(farPlane)
        {
        }

        glm::vec3 GetForward() const
        {
            glm::vec3 forward;
            forward.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
            forward.y = sin(glm::radians(m_pitch));
            forward.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
            return glm::normalize(forward);
        }

        glm::mat4 GetViewMatrix() const
        {
            return glm::lookAt(m_position, m_position + GetForward(), m_up);
        }

        glm::mat4 GetProjectionMatrix() const
        {
            return glm::perspective(glm::radians(m_fovDegrees), m_aspectRatio, m_nearPlane, m_farPlane);
        }

        void MoveForward(float amount)
        {
            m_position += GetForward() * amount;
        }

        void MoveRight(float amount)
        {
            glm::vec3 right = glm::normalize(glm::cross(GetForward(), m_up));
            m_position += right * amount;
        }

        // Accumulates yaw/pitch from mouse deltas. Pitch is clamped so the
        // camera can't flip upside down looking straight up/down.
        void Rotate(float deltaYawDegrees, float deltaPitchDegrees)
        {
            m_yaw += deltaYawDegrees;
            m_pitch = glm::clamp(m_pitch + deltaPitchDegrees, -89.0f, 89.0f);
        }

    private:
        glm::vec3 m_position;
        float m_yaw;   // degrees, rotation around the world Y axis
        float m_pitch; // degrees, rotation up/down, clamped to +/-89
        glm::vec3 m_up;
        float m_fovDegrees;
        float m_aspectRatio;
        float m_nearPlane;
        float m_farPlane;
    };
}
