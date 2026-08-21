#include "Audio/AudioSource.h"
#include "Audio/AudioEngine.h"
#include "Core/Log.h"
#include <miniaudio.h>

namespace Engine
{
    AudioSource::~AudioSource()
    {
        // Le destructeur vit ici et pas dans l'en-tête : ma_sound n'y est qu'une
        // déclaration anticipée, unique_ptr a besoin du type complet pour le détruire.
        if (m_Sound)
            ma_sound_uninit(m_Sound.get());
    }

    std::shared_ptr<AudioSource> AudioSource::LoadFromFile(const std::string &path)
    {
        auto source = std::make_shared<AudioSource>();
        source->m_Path = path;

        ma_engine *engine = AudioEngine::GetNativeEngine();
        if (engine == nullptr)
            return source; // audio indisponible : un son muet, mais utilisable

        auto sound = std::make_unique<ma_sound>();
        const ma_result result = ma_sound_init_from_file(engine, path.c_str(), 0, nullptr, nullptr, sound.get());
        if (result != MA_SUCCESS)
        {
            ENGINE_LOG_ERROR(LogCategories::Audio, "Sound not found or unreadable: {0}", path);
            return source;
        }

        source->m_Sound = std::move(sound);
        return source;
    }

    void AudioSource::Play()
    {
        if (!m_Sound)
            return;

        // Rembobinage explicite : rejouer un son déjà terminé ne repart pas du début
        // tout seul, il resterait à sa position de fin et ne s'entendrait pas.
        ma_sound_seek_to_pcm_frame(m_Sound.get(), 0);
        ma_sound_start(m_Sound.get());
    }

    void AudioSource::Stop()
    {
        if (!m_Sound)
            return;

        ma_sound_stop(m_Sound.get());
        ma_sound_seek_to_pcm_frame(m_Sound.get(), 0);
    }

    void AudioSource::Pause()
    {
        // Un simple stop, sans rembobiner : la lecture reprend là où elle s'est arrêtée.
        if (m_Sound)
            ma_sound_stop(m_Sound.get());
    }

    bool AudioSource::IsPlaying() const
    {
        return m_Sound && ma_sound_is_playing(m_Sound.get());
    }

    void AudioSource::SetLooping(bool looping)
    {
        m_Looping = looping;
        if (m_Sound)
            ma_sound_set_looping(m_Sound.get(), looping ? MA_TRUE : MA_FALSE);
    }

    void AudioSource::SetVolume(float volume)
    {
        m_Volume = volume < 0.0f ? 0.0f : volume;
        if (m_Sound)
            ma_sound_set_volume(m_Sound.get(), m_Volume);
    }
}
