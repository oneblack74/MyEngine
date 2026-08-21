#include "Scene/AudioSystem.h"
#include "Scene/Components.h"
#include "Scene/Entity.h"

namespace Engine
{
    void AudioSystem::OnRuntimeStart(Scene &scene)
    {
        auto view = scene.GetAllEntitiesWith<AudioComponent>();
        for (auto entityHandle : view)
        {
            auto &audio = view.get<AudioComponent>(entityHandle);
            if (audio.Path.empty())
                continue;

            // Un son neuf par exécution, même si la scène a été copiée avec le sien :
            // deux scènes ne doivent pas partager la même instance de lecture.
            audio.Source = AudioSource::LoadFromFile(audio.Path);
            audio.Source->SetVolume(audio.Volume);
            audio.Source->SetLooping(audio.Loop);

            if (audio.PlayOnStart)
                audio.Source->Play();
        }
    }

    void AudioSystem::OnUpdate(Scene &scene)
    {
        auto view = scene.GetAllEntitiesWith<AudioComponent>();
        for (auto entityHandle : view)
        {
            auto &audio = view.get<AudioComponent>(entityHandle);
            if (!audio.Source)
                continue;

            audio.Source->SetVolume(audio.Volume);
            audio.Source->SetLooping(audio.Loop);
        }
    }

    void AudioSystem::OnRuntimeStop(Scene &scene)
    {
        auto view = scene.GetAllEntitiesWith<AudioComponent>();
        for (auto entityHandle : view)
        {
            auto &audio = view.get<AudioComponent>(entityHandle);
            if (!audio.Source)
                continue;

            audio.Source->Stop();
            audio.Source.reset();
        }
    }
}
