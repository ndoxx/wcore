#ifndef SOUND_SYSTEM_H
#define SOUND_SYSTEM_H

#include <filesystem>
#include <map>

#include "game_system.h"

namespace FMOD
{
    class Sound;
}

namespace fs = std::filesystem;

namespace wcore
{

class SoundSystem: public GameSystem
{
public:
    SoundSystem();
    ~SoundSystem();

    virtual void update(const GameClock& clock) override;
    virtual void init_events(InputHandler& handler) override;
#ifndef __DISABLE_EDITOR__
    virtual void generate_widget() override;
#endif

    bool load_soundfx(const char* filename, bool loop=false);
    void play_soundfx(hash_t name,
                      const math::vec3& position = math::vec3(0),
                      const math::vec3& velocity = math::vec3(0));

private:
    float distance_factor_;
    float doppler_scale_;
    float rolloff_scale_;

    math::vec3 last_campos_;

    fs::path soundfx_path_;
    fs::path soundbgm_path_;

    std::map<hash_t, FMOD::Sound*> soundfx_;
};

} // namespace wcore

#endif // SOUND_SYSTEM_H
