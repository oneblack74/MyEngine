add_executable(Editor
    src/main.cpp
    src/EditorLayer.cpp
    src/Panels/ViewportPanel.cpp
)

target_include_directories(Editor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Editor
    PRIVATE
        Engine
        imgui
)
