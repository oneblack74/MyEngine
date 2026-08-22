add_executable(Runtime
    src/main.cpp
    src/RuntimeLayer.cpp
)

target_include_directories(Runtime
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Runtime
    PRIVATE
        Engine
)

# Contrairement à l'éditeur, aucune racine d'assets n'est compilée en dur : un jeu
# packagé emporte ses assets à côté de son exécutable, et c'est le défaut de
# RuntimeOptions ("assets", relatif au dossier de travail).
add_custom_command(TARGET Runtime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:Runtime>/../assets
)
