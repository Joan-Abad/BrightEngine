#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace brightengine
{
    struct Transform
    {
        glm::vec3 position{ 0.0f };
        glm::vec3 rotationEuler{ 0.0f }; // radians, per axis
        glm::vec3 scale{ 1.0f };

        glm::mat4 GetMatrix() const
        {
            glm::mat4 matrix = glm::translate(glm::mat4(1.0f), position);
            matrix = glm::rotate(matrix, rotationEuler.x, glm::vec3(1.0f, 0.0f, 0.0f));
            matrix = glm::rotate(matrix, rotationEuler.y, glm::vec3(0.0f, 1.0f, 0.0f));
            matrix = glm::rotate(matrix, rotationEuler.z, glm::vec3(0.0f, 0.0f, 1.0f));
            matrix = glm::scale(matrix, scale);
            return matrix;
        }
    };
}
