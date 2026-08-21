#include "Audio/AudioEngine.h"
#include "Core/Log.h"
#include <miniaudio.h>

namespace Engine
{
    namespace
    {
        ma_engine s_Engine;
        ma_context s_NullContext;
        bool s_Initialized = false;
        bool s_UsingNullBackend = false;
        float s_MasterVolume = 1.0f;
    }

    void AudioEngine::Init()
    {
        if (s_Initialized)
            return;

        ma_engine_config config = ma_engine_config_init();
        if (ma_engine_init(&config, &s_Engine) == MA_SUCCESS)
        {
            s_Initialized = true;
            ENGINE_LOG_INFO(LogCategories::Audio, "Audio initialisé ({0} Hz)", ma_engine_get_sample_rate(&s_Engine));
            return;
        }

        // Pas de périphérique de sortie utilisable : on retombe sur le backend "null"
        // de miniaudio, qui consomme les sons sans rien émettre. Le jeu tourne alors
        // sans son plutôt que de refuser de démarrer — c'est ce qui permet aussi aux
        // tests automatisés de s'exécuter sur une machine sans carte son.
        ma_backend nullBackend = ma_backend_null;
        if (ma_context_init(&nullBackend, 1, nullptr, &s_NullContext) != MA_SUCCESS)
        {
            ENGINE_LOG_ERROR(LogCategories::Audio, "Audio indisponible : impossible d'initialiser miniaudio");
            return;
        }

        config.pContext = &s_NullContext;
        if (ma_engine_init(&config, &s_Engine) != MA_SUCCESS)
        {
            ma_context_uninit(&s_NullContext);
            ENGINE_LOG_ERROR(LogCategories::Audio, "Audio indisponible : impossible d'initialiser miniaudio");
            return;
        }

        s_Initialized = true;
        s_UsingNullBackend = true;
        ENGINE_LOG_WARN(LogCategories::Audio, "Aucun périphérique audio : les sons seront joués en silence");
    }

    void AudioEngine::Shutdown()
    {
        if (!s_Initialized)
            return;

        ma_engine_uninit(&s_Engine);
        if (s_UsingNullBackend)
            ma_context_uninit(&s_NullContext);

        s_Initialized = false;
        s_UsingNullBackend = false;
    }

    bool AudioEngine::IsAvailable()
    {
        return s_Initialized;
    }

    ma_engine *AudioEngine::GetNativeEngine()
    {
        return s_Initialized ? &s_Engine : nullptr;
    }

    void AudioEngine::SetMasterVolume(float volume)
    {
        // Mémorisé même sans moteur audio : le réglage doit survivre à une machine
        // muette, ne serait-ce que pour être sérialisé ou affiché.
        s_MasterVolume = volume < 0.0f ? 0.0f : volume;

        if (s_Initialized)
            ma_engine_set_volume(&s_Engine, s_MasterVolume);
    }

    float AudioEngine::GetMasterVolume()
    {
        return s_MasterVolume;
    }
}
