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

# Le player se pilote sans écran : il joue quelques frames de la scène de démo puis
# quitte, en signalant par son code de sortie si le chargement a échoué.
add_test(NAME runtime_demo_scene
         COMMAND Runtime --headless --frames 30
                 --assets ${CMAKE_CURRENT_SOURCE_DIR}/assets scenes/demo.scene
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# Un code de sortie 0 ne dit pas que quelque chose a été dessiné : cette variante
# traverse tout le chemin de rendu jusqu'à l'écriture du PNG, qui échoue bruyamment.
add_test(NAME runtime_demo_screenshot
         COMMAND Runtime --headless --screenshot output/captures/runtime_demo.png
                 --assets ${CMAKE_CURRENT_SOURCE_DIR}/assets scenes/demo.scene
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# Une scène manquante doit arrêter le jeu, pas ouvrir une fenêtre vide.
add_test(NAME runtime_missing_scene
         COMMAND Runtime --headless --frames 1
                 --assets ${CMAKE_CURRENT_SOURCE_DIR}/assets scenes/does_not_exist.scene
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(runtime_missing_scene PROPERTIES WILL_FAIL TRUE)

# Export : l'exécutable et ses assets, côte à côte, dans un dossier qu'on peut zipper
# tel quel (`cmake --install build --prefix <dossier> --component runtime`). Le player y
# retrouve ses assets sans dépendre du dossier de travail ni des sources.
install(TARGETS Runtime RUNTIME DESTINATION . COMPONENT runtime)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/assets/ DESTINATION assets COMPONENT runtime)
