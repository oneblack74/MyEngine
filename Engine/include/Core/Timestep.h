#pragma once

namespace Engine
{
    // Enveloppe un delta time en secondes. Type dédié (plutôt qu'un float nu) pour
    // éviter toute ambiguïté secondes-vs-millisecondes côté appelant de OnUpdate.
    class Timestep
    {
    public:
        Timestep(float seconds = 0.0f) : m_Seconds(seconds) {}

        operator float() const { return m_Seconds; }

        float GetSeconds() const { return m_Seconds; }
        float GetMilliseconds() const { return m_Seconds * 1000.0f; }

    private:
        float m_Seconds;
    };
}
