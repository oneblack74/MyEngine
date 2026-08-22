add_executable(MyFirstGame
    src/main.cpp
    src/GameLayer.cpp
)

target_include_directories(MyFirstGame
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Comme l'éditeur, un build de développement travaille sur les assets sources : le jeu
# n'a alors rien à recopier pour tourner. Sans cette définition (build packagé), il
# retombe sur le dossier assets/ posé à côté de l'exécutable.
target_compile_definitions(MyFirstGame PRIVATE
    MYENGINE_GAME_ASSET_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/assets")

target_link_libraries(MyFirstGame
    PRIVATE
        Engine
)
