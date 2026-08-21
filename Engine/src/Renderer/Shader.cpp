#include "Renderer/Shader.h"
#include "Core/Log.h"
#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

namespace Engine
{
    Shader::Shader(const std::string &vertexSrc, const std::string &fragmentSrc)
    {
        // Vertex
        uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
        const char *src = vertexSrc.c_str();
        glShaderSource(vertexShader, 1, &src, nullptr);
        glCompileShader(vertexShader);

        int success;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[512];
            glGetShaderInfoLog(vertexShader, 512, nullptr, log);
            ENGINE_LOG_ERROR(LogCategories::Renderer, "Vertex shader error: {0}", log);
        }

        // Fragment
        uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        src = fragmentSrc.c_str();
        glShaderSource(fragmentShader, 1, &src, nullptr);
        glCompileShader(fragmentShader);

        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[512];
            glGetShaderInfoLog(fragmentShader, 512, nullptr, log);
            ENGINE_LOG_ERROR(LogCategories::Renderer, "Fragment shader error: {0}", log);
        }

        // Link
        m_RendererID = glCreateProgram();
        glAttachShader(m_RendererID, vertexShader);
        glAttachShader(m_RendererID, fragmentShader);
        glLinkProgram(m_RendererID);

        glGetProgramiv(m_RendererID, GL_LINK_STATUS, &success);
        if (!success)
        {
            char log[512];
            glGetProgramInfoLog(m_RendererID, 512, nullptr, log);
            ENGINE_LOG_ERROR(LogCategories::Renderer, "Shader link error: {0}", log);
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    Shader::~Shader()
    {
        glDeleteProgram(m_RendererID);
    }

    void Shader::Bind() const
    {
        glUseProgram(m_RendererID);
    }

    void Shader::Unbind() const
    {
        glUseProgram(0);
    }

    void Shader::SetMat4(const std::string &name, const glm::mat4 &matrix)
    {
        int location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void Shader::SetInt(const std::string &name, int value)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform1i(location, value);
    }

    void Shader::SetIntArray(const std::string &name, int *values, uint32_t count)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform1iv(location, count, values);
    }

}
