#pragma once
#include "Event.h"

namespace Engine
{

    class KeyPressedEvent : public Event
    {
    public:
        KeyPressedEvent(int keycode) : m_KeyCode(keycode) {}

        int GetKeyCode() const { return m_KeyCode; }

        EventType GetType() const override { return EventType::KeyPressed; }
        int GetCategoryFlags() const override { return (int)EventCategory::Keyboard; }
        std::string GetName() const override { return "KeyPressedEvent"; }

    private:
        int m_KeyCode;
    };

    class KeyReleasedEvent : public Event
    {
    public:
        KeyReleasedEvent(int keycode) : m_KeyCode(keycode) {}

        int GetKeyCode() const { return m_KeyCode; }

        EventType GetType() const override { return EventType::KeyReleased; }
        int GetCategoryFlags() const override { return (int)EventCategory::Keyboard; }
        std::string GetName() const override { return "KeyReleasedEvent"; }

    private:
        int m_KeyCode;
    };
}
