add_library(Engine STATIC
    src/Core/Application.cpp
    src/Core/Window.cpp
    src/Core/LayerStack.cpp
)

target_include_directories(Engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(Engine PUBLIC glfw)
