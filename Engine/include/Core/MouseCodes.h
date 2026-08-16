#pragma once

// Correspond volontairement aux codes GLFW_MOUSE_BUTTON_* (GLFW/glfw3.h)
namespace Engine
{
    using MouseCode = int;

    namespace Mouse
    {
        enum : MouseCode
        {
            Button0 = 0,
            Button1 = 1,
            Button2 = 2,
            Button3 = 3,
            Button4 = 4,
            Button5 = 5,
            Button6 = 6,
            Button7 = 7,

            ButtonLeft = Button0,
            ButtonRight = Button1,
            ButtonMiddle = Button2,
        };
    }
}
