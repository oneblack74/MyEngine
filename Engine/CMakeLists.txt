add_library(Engine STATIC
    src/Core/Application.cpp
    src/Core/Window.cpp
    src/Core/LayerStack.cpp
    src/Core/Log.cpp
    src/Core/UUID.cpp
    src/Core/Input.cpp
    src/Renderer/Buffer.cpp
    src/Renderer/VertexArray.cpp
    src/Renderer/Shader.cpp
    src/Renderer/OrthographicCamera.cpp
    src/Renderer/Renderer2D.cpp
    src/Renderer/SubTexture2D.cpp
    src/Renderer/Framebuffer.cpp
    vendor/stb/stb_image.cpp
    src/Utils/ImageCapture.cpp
    src/Renderer/Texture.cpp
    src/Scene/Scene.cpp
    src/Scene/Entity.cpp
    src/Scene/RenderSystem.cpp
    src/Scene/SceneSerializer.cpp
    src/Scene/SceneManager.cpp
    src/Scene/SceneRuntime.cpp
    src/Scene/PhysicsSystem.cpp
    src/Scene/AudioSystem.cpp
    src/Physics/Physics2D.cpp
    src/Assets/AssetTypes.cpp
    src/Assets/AssetManager.cpp
    src/Audio/AudioEngine.cpp
    src/Audio/AudioSource.cpp
)

target_include_directories(Engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb
)

target_link_libraries(Engine PUBLIC
    glfw
    spdlog::spdlog
    glad_gl_core_46
    glm::glm
    EnTT::EnTT
    nlohmann_json::nlohmann_json
    box2d
    miniaudio
)

