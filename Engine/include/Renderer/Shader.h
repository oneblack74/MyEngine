#pragma once
#include <string>
#include <cstdint>
#include <glm/glm.hpp>

namespace Engine
{
    class Shader
    {
    public:
        Shader(const std::string &vertexSrc, const std::string &fragmentSrc);
        ~Shader();

        void Bind() const;
        void Unbind() const;

        void SetMat4(const std::string &name, const glm::mat4 &matrix);
        void SetInt(const std::string &name, int value);
        void SetIntArray(const std::string &name, int *values, uint32_t count);

    private:
        uint32_t m_RendererID;
    };
}
