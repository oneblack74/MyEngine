#pragma once
#include "Core/Layer.h"
#include <vector>

namespace Engine
{
    class LayerStack
    {
    public:
        LayerStack() = default;
        ~LayerStack();

        void PushLayer(Layer *layer);
        void PopLayer(Layer *layer);

        // UI & Debug (always on top)
        void PushOverlay(Layer *overlay);
        void PopOverlay(Layer *overlay);

        std::vector<Layer *>::iterator begin() { return m_Layers.begin(); }
        std::vector<Layer *>::iterator end() { return m_Layers.end(); }

    private:
        std::vector<Layer *> m_Layers;
        unsigned int m_LayerInsertIndex = 0;
    };
}
