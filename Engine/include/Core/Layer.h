#pragma once
#include "Events/Event.h"
#include <string>

namespace Engine
{
    class Layer
    {
    public:
        Layer(const std::string &name = "Layer") : m_Name(name) {}
        virtual ~Layer() = default;

        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate() {}
        virtual void OnEvent(Event &event) {}

        const std::string &GetName() const { return m_Name; }

    protected:
        std::string m_Name;
    };
}
