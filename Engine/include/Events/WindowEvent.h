#pragma once
#include "Event.h"

namespace Engine
{

    class WindowCloseEvent : public Event
    {
    public:
        WindowCloseEvent() {}

        EventType GetType() const override { return EventType::WindowClose; }
        int GetCategoryFlags() const override { return (int)EventCategory::Window; }
        std::string GetName() const override { return "WindowCloseEvent"; }
    };

    class WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(int width, int height)
            : m_Width(width), m_Height(height) {}

        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }

        EventType GetType() const override { return EventType::WindowResize; }
        int GetCategoryFlags() const override { return (int)EventCategory::Window; }
        std::string GetName() const override { return "WindowResizeEvent"; }

    private:
        int m_Width, m_Height;
    };
}
