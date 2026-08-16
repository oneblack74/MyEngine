MyEngine/
│
├── build/
|
├── docs/
|  ├── architecture.md
|  └── roadmap.md
|
├── Editor/
|  ├── src/
|  |  └── main.cpp
|  |
|  └── CMakeLists.txt
|
├── Engine/
|  ├── include/
|  |  ├── Core/
|  |  |  ├── Application.h
|  |  |  ├── Layer.h
|  |  |  ├── LayerStack.h
|  |  |  ├── Log.h
|  |  |  └── Window.h
|  |  |
|  |  ├── Events/
|  |  |  ├── Event.h
|  |  |  ├── KeyEvent.h
|  |  |  └── WindowEvent.h
|  |  |
|  |  └── Renderer/
|  |     ├── Buffer.h
|  |     ├── OrthographicCamera.h
|  |     ├── RenderCommand.h
|  |     ├── Renderer.h
|  |     ├── Shader.h
|  |     ├── Texture.h
|  |     └── VertexArray.h
|  |
|  ├── src/
|  |  ├── Core/
|  |  |  ├── Application.cpp
|  |  |  ├── LayerStack.cpp
|  |  |  ├── Log.cpp
|  |  |  └── Window.cpp
|  |  |
|  |  └── Renderer/
|  |     ├── Buffer.cpp
|  |     ├── OrthographicCamera.cpp
|  |     ├── Shader.cpp
|  |     ├── Texture.cpp
|  |     └── VertexArray.cpp
|  |
|  ├── vendor/
|  |  └── stb/
|  |     ├── stb_image.cpp
|  |     └── stb_image.h
|  |
|  └── CMakeLists.txt
|
├── SandBox/
|  ├── assets/
|  |  └── textures/
|  |     └── axololt.jpg
|  |
|  ├── src/
|  |  └── main.cpp
|  |
|  └── CMakeLists.txt
|
├── .gitattributes
├── .gitignore
├── CMakeLists.txt
└── LICENSE