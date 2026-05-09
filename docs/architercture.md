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
|  |     ├── RenderCommand.h
|  |     └── Renderer.h
|  |
|  ├── src/
|  |  ├── Core/
|  |  |  ├── Application.cpp
|  |  |  ├── LayerStack.cpp
|  |  |  ├── Log.cpp
|  |  |  └── Window.cpp
|  |  |
|  |  └── Renderer/
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