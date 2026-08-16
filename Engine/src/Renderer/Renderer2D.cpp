#include "Renderer/Renderer2D.h"
#include "Renderer/VertexArray.h"
#include "Renderer/Buffer.h"
#include "Renderer/Shader.h"
#include "Renderer/RenderCommand.h"
#include <glm/gtc/matrix_transform.hpp>
#include <array>

namespace Engine
{
    struct QuadVertex
    {
        glm::vec3 Position;
        glm::vec4 Color;
        glm::vec2 TexCoord;
        float TexIndex;
        float TilingFactor;
    };

    struct Renderer2DData
    {
        static const uint32_t MaxQuads = 10000;
        static const uint32_t MaxVertices = MaxQuads * 4;
        static const uint32_t MaxIndices = MaxQuads * 6;
        static const uint32_t MaxTextureSlots = 32;

        std::shared_ptr<VertexArray> QuadVertexArray;
        std::shared_ptr<VertexBuffer> QuadVertexBuffer;
        std::shared_ptr<Shader> TextureShader;
        std::shared_ptr<Texture2D> WhiteTexture;

        uint32_t QuadIndexCount = 0;
        QuadVertex *QuadVertexBufferBase = nullptr;
        QuadVertex *QuadVertexBufferPtr = nullptr;

        std::array<std::shared_ptr<Texture2D>, MaxTextureSlots> TextureSlots;
        uint32_t TextureSlotIndex = 1; // slot 0 réservé à la texture blanche

        glm::vec4 QuadVertexPositions[4];
    };

    static Renderer2DData s_Data;

    void Renderer2D::Init()
    {
        s_Data.QuadVertexArray = std::make_shared<VertexArray>();

        s_Data.QuadVertexBuffer = std::make_shared<VertexBuffer>(Renderer2DData::MaxVertices * sizeof(QuadVertex));
        s_Data.QuadVertexBuffer->SetLayout({
            {ShaderDataType::Float3, "a_Position"},
            {ShaderDataType::Float4, "a_Color"},
            {ShaderDataType::Float2, "a_TexCoord"},
            {ShaderDataType::Float, "a_TexIndex"},
            {ShaderDataType::Float, "a_TilingFactor"},
        });
        s_Data.QuadVertexArray->AddVertexBuffer(s_Data.QuadVertexBuffer);

        s_Data.QuadVertexBufferBase = new QuadVertex[Renderer2DData::MaxVertices];

        uint32_t *quadIndices = new uint32_t[Renderer2DData::MaxIndices];
        uint32_t offset = 0;
        for (uint32_t i = 0; i < Renderer2DData::MaxIndices; i += 6)
        {
            quadIndices[i + 0] = offset + 0;
            quadIndices[i + 1] = offset + 1;
            quadIndices[i + 2] = offset + 2;
            quadIndices[i + 3] = offset + 2;
            quadIndices[i + 4] = offset + 3;
            quadIndices[i + 5] = offset + 0;
            offset += 4;
        }

        auto quadIB = std::make_shared<IndexBuffer>(quadIndices, Renderer2DData::MaxIndices);
        s_Data.QuadVertexArray->SetIndexBuffer(quadIB);
        delete[] quadIndices;

        // Texture blanche 1x1 : sert de "texture" pour les quads dessinés en couleur pleine
        s_Data.WhiteTexture = std::make_shared<Texture2D>(1, 1);
        uint32_t whiteTextureData = 0xffffffff;
        s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

        int samplers[Renderer2DData::MaxTextureSlots];
        for (uint32_t i = 0; i < Renderer2DData::MaxTextureSlots; i++)
            samplers[i] = (int)i;

        std::string vertexSrc = R"(
            #version 330 core
            layout(location = 0) in vec3 a_Position;
            layout(location = 1) in vec4 a_Color;
            layout(location = 2) in vec2 a_TexCoord;
            layout(location = 3) in float a_TexIndex;
            layout(location = 4) in float a_TilingFactor;

            uniform mat4 u_ViewProjection;

            out vec4 v_Color;
            out vec2 v_TexCoord;
            out float v_TexIndex;
            out float v_TilingFactor;

            void main()
            {
                v_Color = a_Color;
                v_TexCoord = a_TexCoord;
                v_TexIndex = a_TexIndex;
                v_TilingFactor = a_TilingFactor;
                gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
            }
        )";

        std::string fragmentSrc = R"(
            #version 330 core
            in vec4 v_Color;
            in vec2 v_TexCoord;
            in float v_TexIndex;
            in float v_TilingFactor;

            out vec4 color;

            uniform sampler2D u_Textures[32];

            void main()
            {
                color = texture(u_Textures[int(v_TexIndex)], v_TexCoord * v_TilingFactor) * v_Color;
            }
        )";

        s_Data.TextureShader = std::make_shared<Shader>(vertexSrc, fragmentSrc);
        s_Data.TextureShader->Bind();
        s_Data.TextureShader->SetIntArray("u_Textures", samplers, Renderer2DData::MaxTextureSlots);

        s_Data.TextureSlots[0] = s_Data.WhiteTexture;

        s_Data.QuadVertexPositions[0] = {-0.5f, -0.5f, 0.0f, 1.0f};
        s_Data.QuadVertexPositions[1] = {0.5f, -0.5f, 0.0f, 1.0f};
        s_Data.QuadVertexPositions[2] = {0.5f, 0.5f, 0.0f, 1.0f};
        s_Data.QuadVertexPositions[3] = {-0.5f, 0.5f, 0.0f, 1.0f};
    }

    void Renderer2D::Shutdown()
    {
        delete[] s_Data.QuadVertexBufferBase;
        s_Data = Renderer2DData();
    }

    void Renderer2D::StartBatch()
    {
        s_Data.QuadIndexCount = 0;
        s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;
        s_Data.TextureSlotIndex = 1;
    }

    void Renderer2D::NextBatch()
    {
        Flush();
        StartBatch();
    }

    void Renderer2D::BeginScene(const OrthographicCamera &camera)
    {
        s_Data.TextureShader->Bind();
        s_Data.TextureShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());

        StartBatch();
    }

    void Renderer2D::EndScene()
    {
        Flush();
    }

    void Renderer2D::Flush()
    {
        if (s_Data.QuadIndexCount == 0)
            return; // rien à dessiner

        uint32_t dataSize = (uint32_t)((uint8_t *)s_Data.QuadVertexBufferPtr - (uint8_t *)s_Data.QuadVertexBufferBase);
        s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, dataSize);

        for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
            s_Data.TextureSlots[i]->Bind(i);

        RenderCommand::DrawIndexed(s_Data.QuadVertexArray, s_Data.QuadIndexCount);
    }

    void Renderer2D::DrawQuad(const glm::vec2 &position, const glm::vec2 &size, const glm::vec4 &color)
    {
        DrawQuad({position.x, position.y, 0.0f}, size, color);
    }

    void Renderer2D::DrawQuad(const glm::vec3 &position, const glm::vec2 &size, const glm::vec4 &color)
    {
        DrawQuad(position, size, s_Data.WhiteTexture, 1.0f, color);
    }

    void Renderer2D::DrawQuad(const glm::vec2 &position, const glm::vec2 &size,
                               const std::shared_ptr<Texture2D> &texture,
                               float tilingFactor, const glm::vec4 &tintColor)
    {
        DrawQuad({position.x, position.y, 0.0f}, size, texture, tilingFactor, tintColor);
    }

    void Renderer2D::DrawQuad(const glm::vec3 &position, const glm::vec2 &size,
                               const std::shared_ptr<Texture2D> &texture,
                               float tilingFactor, const glm::vec4 &tintColor)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
                               glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
        DrawQuad(transform, texture, tilingFactor, tintColor);
    }

    void Renderer2D::DrawQuad(const glm::vec2 &position, const glm::vec2 &size,
                               const std::shared_ptr<SubTexture2D> &subTexture,
                               float tilingFactor, const glm::vec4 &tintColor)
    {
        DrawQuad({position.x, position.y, 0.0f}, size, subTexture, tilingFactor, tintColor);
    }

    void Renderer2D::DrawQuad(const glm::vec3 &position, const glm::vec2 &size,
                               const std::shared_ptr<SubTexture2D> &subTexture,
                               float tilingFactor, const glm::vec4 &tintColor)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
                               glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
        DrawQuadTexture(transform, subTexture->GetTexture(), subTexture->GetTexCoords(), tilingFactor, tintColor);
    }

    void Renderer2D::DrawQuad(const glm::mat4 &transform, const glm::vec4 &color)
    {
        DrawQuad(transform, s_Data.WhiteTexture, 1.0f, color);
    }

    void Renderer2D::DrawQuad(const glm::mat4 &transform, const std::shared_ptr<Texture2D> &texture,
                               float tilingFactor, const glm::vec4 &tintColor)
    {
        glm::vec2 texCoords[4] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
        DrawQuadTexture(transform, texture, texCoords, tilingFactor, tintColor);
    }

    void Renderer2D::DrawQuadTexture(const glm::mat4 &transform,
                                      const std::shared_ptr<Texture2D> &texture,
                                      const glm::vec2 *texCoords,
                                      float tilingFactor, const glm::vec4 &tintColor)
    {
        if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
            NextBatch();

        float textureIndex = 0.0f;
        for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
        {
            if (s_Data.TextureSlots[i]->GetRendererID() == texture->GetRendererID())
            {
                textureIndex = (float)i;
                break;
            }
        }

        if (textureIndex == 0.0f)
        {
            if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
                NextBatch();

            textureIndex = (float)s_Data.TextureSlotIndex;
            s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
            s_Data.TextureSlotIndex++;
        }

        for (int i = 0; i < 4; i++)
        {
            s_Data.QuadVertexBufferPtr->Position = glm::vec3(transform * s_Data.QuadVertexPositions[i]);
            s_Data.QuadVertexBufferPtr->Color = tintColor;
            s_Data.QuadVertexBufferPtr->TexCoord = texCoords[i];
            s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
            s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
            s_Data.QuadVertexBufferPtr++;
        }

        s_Data.QuadIndexCount += 6;
    }
}
