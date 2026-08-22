cmake_minimum_required(VERSION 3.20)
project(MyEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Les sources sont en UTF-8 (commentaires accentués) : sans ce flag, MSVC les lit dans
# la page de codes locale et se plaint, là où GCC et Clang supposent déjà l'UTF-8.
if(MSVC)
    add_compile_options(/utf-8)
endif()

include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.8
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.12.0
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        glad2
)
FetchContent_MakeAvailable(glad)

add_subdirectory("${glad_SOURCE_DIR}/cmake" glad_cmake)
glad_add_library(glad_gl_core_46 REPRODUCIBLE API gl:core=4.6)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.3
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        v3.13.2
)
FetchContent_MakeAvailable(entt)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
    box2d
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    GIT_TAG        v3.1.1
)
set(BOX2D_SAMPLES OFF CACHE BOOL "" FORCE)
set(BOX2D_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_DOCS OFF CACHE BOOL "" FORCE)
set(BOX2D_PROFILE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(box2d)

FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG        0.11.25
)
# Contrairement à ImGui, miniaudio fournit son propre CMakeLists définissant une cible
# "miniaudio" qui compile déjà l'implémentation — rien à ajouter de notre côté. Ses
# exemples et ses tests sont écartés, on ne veut que la bibliothèque.
set(MINIAUDIO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MINIAUDIO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(miniaudio)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui)

# ImGui ne fournit pas de CMakeLists.txt utilisable directement : on définit
# nous-mêmes une lib statique avec le coeur + les backends GLFW/OpenGL3.
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD2)
target_link_libraries(imgui PUBLIC glfw glad_gl_core_46)

# Dear ImGui Test Engine : automatisation de l'UI de l'éditeur (clic par label,
# assertions, captures). Optionnel — sa licence est gratuite pour un usage personnel
# ou open-source mais payante en usage commercial, donc on doit pouvoir construire
# un éditeur qui ne l'embarque pas du tout.
option(MYENGINE_EDITOR_TESTS "Compiler l'éditeur avec Dear ImGui Test Engine" ON)

# Au niveau racine, et pas seulement sous MYENGINE_EDITOR_TESTS : les tests du Runtime
# ne dépendent pas du Test Engine, ils doivent tourner même sans lui.
enable_testing()

if(MYENGINE_EDITOR_TESTS)
    FetchContent_Declare(
        imgui_test_engine
        GIT_REPOSITORY https://github.com/ocornut/imgui_test_engine.git
        GIT_TAG        main
    )
    FetchContent_MakeAvailable(imgui_test_engine)

    # Le Test Engine s'accroche à ImGui via des hooks compilés dans imgui.cpp lui-même :
    # sans cette définition sur la cible imgui, ItemClick & co. ne voient jamais aucun
    # widget. PUBLIC pour que l'éditeur la reçoive aussi (ses en-têtes en dépendent).
    target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_TEST_ENGINE)

    # Pas de CMakeLists exploitable non plus de ce côté : on définit la lib nous-mêmes.
    add_library(imgui_test_engine STATIC
        ${imgui_test_engine_SOURCE_DIR}/imgui_test_engine/imgui_capture_tool.cpp
        ${imgui_test_engine_SOURCE_DIR}/imgui_test_engine/imgui_te_context.cpp
        ${imgui_test_engine_SOURCE_DIR}/imgui_test_engine/imgui_te_coroutine.cpp
        ${imgui_test_engine_SOURCE_DIR}/imgui_test_engine/imgui_te_engine.cpp
        ${imgui_test_engine_SOURCE_DIR}/imgui_test_engine/imgui_te_exporters.cpp
        ${imgui_test_engine_SOURCE_DIR}/imgui_test_engine/imgui_te_perftool.cpp
        ${imgui_test_engine_SOURCE_DIR}/imgui_test_engine/imgui_te_ui.cpp
        ${imgui_test_engine_SOURCE_DIR}/imgui_test_engine/imgui_te_utils.cpp
    )
    target_include_directories(imgui_test_engine PUBLIC
        ${imgui_test_engine_SOURCE_DIR}
        ${imgui_test_engine_SOURCE_DIR}/imgui_test_engine
    )
    # Permet d'écrire les tests comme des lambdas capturantes (elles reçoivent ainsi
    # l'EditorLayer) plutôt que des pointeurs de fonction nus. PUBLIC : la définition
    # change la signature des champs du Test Engine, elle doit être identique partout.
    target_compile_definitions(imgui_test_engine PUBLIC IMGUI_TEST_ENGINE_ENABLE_STD_FUNCTION=1)
    # Sans ça, ImGuiTestEngineIO::CoroutineFuncs reste nul et ImGuiTestEngine_Start()
    # déréférence un pointeur nul (l'assert qui le signale est désactivé en Release).
    target_compile_definitions(imgui_test_engine PUBLIC IMGUI_TEST_ENGINE_ENABLE_COROUTINE_STDTHREAD_IMPL=1)
    # Même piège : sans cette définition les captures d'écran demandées par un test sont
    # silencieusement ignorées ("Skipped screenshot capture").
    target_compile_definitions(imgui_test_engine PUBLIC IMGUI_TEST_ENGINE_ENABLE_CAPTURE=1)
    target_link_libraries(imgui_test_engine PUBLIC imgui)
    find_package(Threads REQUIRED)
    target_link_libraries(imgui_test_engine PUBLIC Threads::Threads)
endif()

FetchContent_Declare(
    imguizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(imguizmo)

# ImGuizmo fournit son propre CMakeLists.txt (target "imguizmo") mais ne lie pas
# ImGui lui-même quand utilisé en sous-projet : on complète le lien nous-mêmes.
target_link_libraries(imguizmo PUBLIC imgui)

add_subdirectory(Engine)
add_subdirectory(Editor)
add_subdirectory(Sandbox)
add_subdirectory(Runtime)
