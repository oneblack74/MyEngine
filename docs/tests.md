# Tests automatisés

L'éditeur peut être piloté sans souris ni écran : il sait se lancer avec une fenêtre
cachée, jouer une suite de tests qui cliquent réellement dans son UI, et capturer son
rendu en PNG. C'est ce qui permet de vérifier un changement sans avoir à le reproduire
à la main à chaque fois.

## Lancer les tests

Depuis la racine du dépôt :

```bash
cmake --build build -j$(nproc) && (cd build && ctest --output-on-failure)
```

Ou directement, ce qui donne le log détaillé de chaque action jouée :

```bash
cd build && ./Editor/Editor --tests-headless
```

Le code de sortie vaut 0 si tout passe, 1 sinon. `--filter` restreint la suite : le
motif est une **sous-chaîne** du nom ou de la catégorie du test, pas un glob — `--filter
inspector` joue toute la catégorie, `--filter reset` tous les tests dont le nom contient
« reset ». `^` ancre au début, `$` à la fin, `-` exclut, `,` sépare plusieurs motifs.

Pour **voir** les tests se jouer, l'éditeur s'ouvre normalement avec la fenêtre du
Test Engine, d'où on lance les tests un par un :

```bash
cd build && ./Editor/Editor --tests
```

## Captures d'écran

Sans tests, pour photographier l'éditeur tel qu'il démarre :

```bash
cd build && ./Editor/Editor --headless --screenshot capture.png
```

Un test peut aussi capturer une fenêtre précise (voir `capture/inspector_panel` dans
`Editor/src/Testing/EditorTests.cpp`) ; les images atterrissent dans
`build/output/captures/`.

## Écrire un test

Tout se passe dans `Editor/src/Testing/EditorTests.cpp`. Un test pilote l'UI puis
vérifie l'état réel de l'éditeur derrière :

```cpp
t = IM_REGISTER_TEST(engine, "inspector", "reset_transform_to_defaults");
t->TestFunc = [&editor](ImGuiTestContext *ctx)
{
    Engine::Entity square = SelectEntity(ctx, editor, "Square");
    ctx->SetRef("Inspecteur");
    ctx->ItemClick(ResetButtonOf("Transform").c_str());
    IM_CHECK(NearlyEqual(square.GetComponent<Engine::TransformComponent>().Position.x, 0.0f));
};
```

Les widgets s'adressent par leur chemin dans la pile d'ID ImGui, `"Fenêtre/Section/Label"`,
avec `"/$$n"` pour la n-ième composante d'un `DragFloat2`/`DragFloat3`. Deux pièges
rencontrés :

- Les entités de la hiérarchie tirent leur ID de leur handle et non de leur nom (deux
  entités peuvent s'appeler pareil) : elles se retrouvent via `ctx->GatherItems` puis
  une comparaison sur `DebugLabel` — c'est ce que fait l'helper `SelectEntity`.
- Un test qui « passe » ne prouve pas qu'il a fait quelque chose. La capture d'écran a
  d'abord été ignorée en silence faute de `IMGUI_TEST_ENGINE_ENABLE_CAPTURE` ; le test
  vérifie donc maintenant que le fichier existe vraiment.

## Tester le runtime

Le player se pilote de la même façon : `--headless` le fait tourner sans rien afficher,
`--frames` lui dit au bout de combien de frames quitter, et son code de sortie vaut 1 si
la scène ne s'est pas chargée ou si la capture n'a pas pu être écrite.

```bash
cd build && ./Runtime/Runtime --headless --frames 30 scenes/demo.scene
cd build && ./Runtime/Runtime --headless --screenshot output/captures/runtime_demo.png scenes/demo.scene
```

Trois tests ctest en découlent (`runtime_demo_scene`, `runtime_demo_screenshot`,
`runtime_missing_scene`), tous joués sur `Runtime/assets/scenes/demo.scene`. Ils ne
dépendent pas du Test Engine — il n'y a pas d'UI à piloter — donc ils tournent aussi
dans un build construit sans lui. Un code de sortie 0 ne prouvant pas qu'une image a
été dessinée, la variante `--screenshot` traverse tout le chemin de rendu jusqu'à
l'écriture du PNG ; et la scène manquante est attendue en échec (`WILL_FAIL`), pour que
l'erreur reste une erreur.

## Construire sans le Test Engine

[Dear ImGui Test Engine](https://github.com/ocornut/imgui_test_engine) est gratuit pour
un usage personnel ou open-source, mais payant en usage commercial. L'option CMake
`MYENGINE_EDITOR_TESTS` (activée par défaut) permet donc de construire un éditeur qui
ne l'embarque pas du tout :

```bash
cmake -S . -B build-release -DMYENGINE_EDITOR_TESTS=OFF
```

Dans ce mode, `Testing/EditorTestEngine.cpp` se compile à vide et les options
`--tests`/`--tests-headless` ne font rien. `--headless` et `--screenshot`, eux, restent
disponibles : ils ne dépendent que du moteur.
