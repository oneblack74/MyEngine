#pragma once

struct ma_engine;

namespace Engine
{
    // Point d'entrée audio : possède le moteur miniaudio et le périphérique de sortie.
    // Une seule instance pour toute l'application, comme Renderer2D.
    class AudioEngine
    {
    public:
        static void Init();
        static void Shutdown();

        // False quand aucun périphérique de sortie n'a pu être ouvert. Le reste du
        // moteur continue de tourner normalement : les sons ne font simplement rien.
        // C'est le cas d'une machine sans carte son, ou d'une exécution automatisée.
        static bool IsAvailable();

        // Nul tant qu'Init n'a pas réussi — AudioSource s'en sert pour charger ses sons.
        static ma_engine *GetNativeEngine();

        // Volume global, appliqué par-dessus celui de chaque son. 1 = volume nominal.
        static void SetMasterVolume(float volume);
        static float GetMasterVolume();
    };
}
