#pragma once
#include <string>
#include <functional>

namespace Engine
{

    enum class EventType
    {
        None = 0,
        WindowClose,
        WindowResize,
        KeyPressed,
        KeyReleased,
        MouseMoved,
        MouseButtonPressed,
        MouseButtonReleased,
        MouseScrolled
    };

    enum class EventCategory
    {
        None = 0,
        Window = 1 << 0,     // 0001
        Keyboard = 1 << 1,   // 0010
        Mouse = 1 << 2,      // 0100
        MouseButton = 1 << 3 // 1000
    };

    class Event
    {
    public:
        bool Handled = false;

        virtual EventType GetType() const = 0;
        virtual int GetCategoryFlags() const = 0;
        virtual std::string GetName() const = 0;

        bool IsInCategory(EventCategory category)
        {
            return GetCategoryFlags() & (int)category;
        }
    };
}
