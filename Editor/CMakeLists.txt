add_executable(Editor
    src/main.cpp
    src/EditorLayer.cpp
    src/Panels/ViewportPanel.cpp
    src/Panels/SceneHierarchyPanel.cpp
    src/Panels/InspectorPanel.cpp
    src/Panels/ContentBrowserPanel.cpp
    src/Panels/ConsolePanel.cpp
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

add_custom_command(TARGET Editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:Editor>/../assets
)
