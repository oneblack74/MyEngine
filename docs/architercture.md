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
|  |     └── VertexArray.cpp
|  |
|  └── CMakeLists.txt
|
├── SandBox/
|  ├── src/
|  |  └── main.cpp
|  |
|  └── CMakeLists.txt
|
├── .gitattributes
├── .gitignore
├── CMakeLists.txt
└── LICENSE