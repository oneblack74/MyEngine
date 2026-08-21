add_executable(Editor
    src/main.cpp
    src/EditorLayer.cpp
    src/Panels/ViewportPanel.cpp
    src/Panels/GamePanel.cpp
    src/Panels/SceneHierarchyPanel.cpp
    src/Panels/InspectorPanel.cpp
    src/Panels/ContentBrowserPanel.cpp
    src/Panels/SceneFileDialog.cpp
    src/Panels/ConsolePanel.cpp
    src/Commands/CommandHistory.cpp
    src/Commands/SceneCommands.cpp
    src/Testing/EditorTestEngine.cpp
    src/Testing/EditorTests.cpp
)

target_include_directories(Editor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# L'éditeur travaille directement sur les assets sources : le registre y est écrit et
# donc versionné avec eux (les handles survivent à un clone), et modifier une image se
# voit tout de suite sans attendre une recopie.
target_compile_definitions(Editor PRIVATE
    MYENGINE_EDITOR_ASSET_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/assets")

target_link_libraries(Editor
    PRIVATE
        Engine
        imgui
        imguizmo
)

if(MYENGINE_EDITOR_TESTS)
    target_link_libraries(Editor PRIVATE imgui_test_engine)
    target_compile_definitions(Editor PRIVATE MYENGINE_EDITOR_TESTS)

    # WORKING_DIRECTORY à la racine du build : l'éditeur charge ses assets par chemin
    # relatif au dossier de travail, il ne tourne pas correctement depuis Editor/.
    add_test(NAME editor_ui_tests
             COMMAND Editor --tests-headless
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

add_custom_command(TARGET Editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:Editor>/../assets
)
