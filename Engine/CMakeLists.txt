add_library(Engine STATIC
    src/Core/Application.cpp
    src/Core/Window.cpp
    src/Core/LayerStack.cpp
    src/Core/Log.cpp
    src/Core/UUID.cpp
    src/Renderer/Buffer.cpp
    src/Renderer/VertexArray.cpp
    src/Renderer/Shader.cpp
    src/Renderer/OrthographicCamera.cpp
    src/Renderer/Renderer2D.cpp
    src/Renderer/SubTexture2D.cpp
    vendor/stb/stb_image.cpp
    src/Renderer/Texture.cpp
    src/Scene/Scene.cpp
    src/Scene/Entity.cpp
    src/Scene/RenderSystem.cpp
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
)

