#ifndef SOUND_SYSTEM_H
#define SOUND_SYSTEM_H

#include <filesystem>
#include <memory>
#include <string>
#include <map>

#include "game_system.h"

namespace fs = std::filesystem;

namespace wcore
{

class SoundSystem: public GameSystem
{
public:
    struct SoundDescriptor
    {
        SoundDescriptor(const char* filename);

        std::string filename;
        float volume_dB;
        float min_distance;
        float max_distance;
        bool loop;
        bool stream;
        bool is3d;
        bool isfx;
    };

    SoundSystem();
    ~SoundSystem();

    virtual void update(const GameClock& clock) override;
    virtual void init_events(InputHandler& handler) override;
#ifndef __DISABLE_EDITOR__
    virtual void generate_widget() override;
#endif

    void register_sound(const SoundDescriptor& descriptor, hash_t name=0);
    bool load_sound(hash_t name);
    bool unload_sound(hash_t name);

    int play_sound(hash_t name,
                   const math::vec3& position = math::vec3(0),
                   const math::vec3& velocity = math::vec3(0),
                   float volume_dB=0.f);

private:
    struct SoundEngineImpl;
    std::unique_ptr<SoundEngineImpl> pimpl_; // opaque pointer

    float distance_factor_;
    float doppler_scale_;
    float rolloff_scale_;

    math::vec3 last_campos_;

    fs::path soundfx_path_;
    fs::path soundbgm_path_;

    std::map<hash_t, SoundDescriptor> descriptors_;
};
/*
inline bool SoundSystem::load_soundfx(const char* filename, bool loop, bool stream, bool is3d)
{
    return load_soundfx(filename, H_(filename), loop, stream, is3d);
}*/

} // namespace wcore

#endif // SOUND_SYSTEM_H
