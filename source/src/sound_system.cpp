#include "scene.h"
#include "sound_system.h"
#include "input_handler.h"
#include "game_clock.h"
#include "camera.h"
#include "config.h"
#include "error.h"
#include "logger.h"

#include "vendor/fmod/fmod.hpp"
#ifdef __DEBUG__
    #include "vendor/fmod/fmod_errors.h"
#endif

namespace wcore
{

inline FMOD_VECTOR to_fmod_vec(const math::vec3& input)
{
    return { input.x(), input.y(), input.z() };
}

static FMOD::System *fmodsys;

#ifdef __DEBUG__
    void ERRCHECK_fn(FMOD_RESULT result, const char *file, int line)
    {
        if(result != FMOD_OK)
        {
            DLOGE("[SoundSystem] FMOD error " + std::to_string(result) + ":", "sound", Severity::CRIT);
            DLOGI("file: " + std::string(file) + " line " + std::to_string(line), "sound", Severity::CRIT);
            DLOGI(FMOD_ErrorString(result), "sound", Severity::CRIT);
        }
    }
    #define ERRCHECK( result ) ERRCHECK_fn( result, __FILE__, __LINE__ )
#else
    #define ERRCHECK( result ) result
#endif

SoundSystem::SoundSystem():
distance_factor_(1.0f),
doppler_scale_(1.0f),
rolloff_scale_(1.0f),
last_campos_(0.f)
{
    DLOGS("[SoundSystem] Initializing.", "sound", Severity::LOW);
    DLOGN("Retrieving config data.", "sound", Severity::LOW);

    // * Retrieve config data
    // Mandatory stuff
    if(!CONFIG.get(H_("root.folders.soundfx"), soundfx_path_))
    {
        DLOGE("[SoundSystem] Cannot find config path 'soundfx'.", "sound", Severity::CRIT);
        fatal();
    }
    if(!CONFIG.get(H_("root.folders.soundbgm"), soundbgm_path_))
    {
        DLOGE("[SoundSystem] Cannot find config path 'soundbgm'.", "sound", Severity::CRIT);
        fatal();
    }
    if(!fs::exists(soundfx_path_))
    {
        DLOGE("[SoundSystem] Cannot find 'soundfx' folder.", "sound", Severity::CRIT);
        fatal();
    }
    if(!fs::exists(soundbgm_path_))
    {
        DLOGE("[SoundSystem] Cannot find 'soundbgm' folder.", "sound", Severity::CRIT);
        fatal();
    }

    // Optional stuff
    uint32_t max_channels = 100;
    CONFIG.get(H_("root.sound.general.max_channels"), max_channels);
    CONFIG.get(H_("root.sound.general.distance_factor"), distance_factor_);
    CONFIG.get(H_("root.sound.general.doppler_scale"), doppler_scale_);
    CONFIG.get(H_("root.sound.general.rolloff_scale"), rolloff_scale_);

    // * Initialize FMOD
    DLOGN("Initializing FMOD.", "sound", Severity::LOW);
    void* extradriverdata = nullptr;
    unsigned int version;
    ERRCHECK(FMOD::System_Create(&fmodsys));
    ERRCHECK(fmodsys->getVersion(&version));
    assert(version < FMOD_VERSION && "FMOD header/lib version mismatch.");
    ERRCHECK(fmodsys->init(max_channels, FMOD_INIT_NORMAL, extradriverdata));
    ERRCHECK(fmodsys->set3DSettings(doppler_scale_, distance_factor_, rolloff_scale_));

    DLOGES("sound", Severity::LOW);
}

SoundSystem::~SoundSystem()
{
    ERRCHECK(fmodsys->close());
    ERRCHECK(fmodsys->release());
}

void SoundSystem::update(const GameClock& clock)
{
    // * First frame -> frame duration = 0.f -> do nothing
    if(clock.get_frame_duration()>0.f)
    {
        // * Get camera position / axes
        auto pscene = locate<Scene>(H_("Scene"));
        auto cam = pscene->get_camera();

        const math::vec3& campos = cam->get_position();

        math::vec3 camvel((campos-last_campos_) * 1.f/clock.get_frame_duration());
        last_campos_ = campos;

        FMOD_VECTOR listenerpos = to_fmod_vec(campos);
        FMOD_VECTOR listenervel = to_fmod_vec(camvel);
        FMOD_VECTOR forward     = to_fmod_vec(cam->get_forward());
        FMOD_VECTOR up          = to_fmod_vec(cam->get_up());
        ERRCHECK(fmodsys->set3DListenerAttributes(0, &listenerpos, &listenervel, &forward, &up));
    }
}

void SoundSystem::init_events(InputHandler& handler)
{

}

#ifndef __DISABLE_EDITOR__
void SoundSystem::generate_widget()
{

}
#endif

bool SoundSystem::load_soundfx(const char* filename, bool loop)
{
    fs::path filepath = soundfx_path_ / filename;
    if(!fs::exists(filepath))
    {
        DLOGE("[SoundSystem] Cannot find sound fx: ", "sound", Severity::CRIT);
        DLOGI(filepath.string(), "sound", Severity::CRIT);
        return false;
    }

    hash_t hname = H_(filename);
    if(soundfx_.find(hname) != soundfx_.end())
    {
        DLOGE("[SoundSystem] Already loaded sound fx (or possible collision): ", "sound", Severity::WARN);
        DLOGI(filepath.string(), "sound", Severity::WARN);
        DLOGI("Skipping.", "sound", Severity::WARN);
        return false;
    }

    FMOD::Sound* out_sound;
    ERRCHECK(fmodsys->createSound(filepath.string().c_str(), FMOD_3D, 0, &out_sound));
    ERRCHECK(out_sound->set3DMinMaxDistance(0.5f * distance_factor_, 5000.0f * distance_factor_));
    if(loop) ERRCHECK(out_sound->setMode(FMOD_LOOP_NORMAL));

    soundfx_.insert(std::pair(hname, out_sound));
    return true;
}

} // namespace wcore
