#pragma once
#include <memory>
#include <string>

struct ma_sound;

namespace Engine
{
    // Un son chargé, prêt à être joué. Enveloppe un ma_sound de miniaudio.
    //
    // Volume et bouclage sont mémorisés côté C++ en plus d'être poussés dans miniaudio :
    // ils doivent rester lisibles quand aucun périphérique audio n'est disponible et
    // qu'il n'y a donc pas de son sous-jacent.
    class AudioSource
    {
    public:
        AudioSource() = default;
        ~AudioSource();

        // Un son possède une ressource miniaudio non copiable.
        AudioSource(const AudioSource &) = delete;
        AudioSource &operator=(const AudioSource &) = delete;

        // Renvoie un son valide même si le fichier n'a pas pu être chargé : les appels
        // de lecture ne feront alors rien. À l'appelant de vérifier IsLoaded() s'il veut
        // distinguer les deux cas.
        static std::shared_ptr<AudioSource> LoadFromFile(const std::string &path);

        void Play();
        void Stop();
        void Pause();
        bool IsPlaying() const;

        void SetLooping(bool looping);
        bool IsLooping() const { return m_Looping; }

        void SetVolume(float volume);
        float GetVolume() const { return m_Volume; }

        bool IsLoaded() const { return m_Sound != nullptr; }
        const std::string &GetPath() const { return m_Path; }

    private:
        std::unique_ptr<ma_sound> m_Sound;
        std::string m_Path;
        float m_Volume = 1.0f;
        bool m_Looping = false;
    };
}
