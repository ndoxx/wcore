#include <map>
#include <sstream>
#include <filesystem>

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

#ifndef __DISABLE_EDITOR__
    #include "imgui/imgui.h"
    #include "gui_utils.h"
#endif

namespace fs = std::filesystem;

namespace wcore
{

static fs::path SOUND_FX_PATH;
static fs::path SOUND_BGM_PATH;

inline FMOD_VECTOR to_fmod_vec(const math::vec3& input)
{
    return { input.x(), input.y(), input.z() };
}

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

struct SoundSystem::SoundEngineImpl
{
    struct Channel;

    SoundEngineImpl():
    fmodsys(nullptr),
    next_channel_id(0)
    {

    }

    typedef std::map<hash_t, FMOD::Sound*> SoundMap;
    //typedef std::map<int, FMOD::Channel*> ChannelMap;
    typedef std::map<int, std::unique_ptr<Channel>> ChannelMap;

    FMOD::System* fmodsys;
    SoundMap      sounds;
    ChannelMap    channels;
    int           next_channel_id;
};

inline float db_to_linear(float value_dB)
{
    return pow(10.f, value_dB/20.f);
}

struct SoundSystem::SoundEngineImpl::Channel
{
    enum class State
    {
        INITIALIZING, LOADING, PREPARING, PLAYING, STOPPING, STOPPED
    };

    Channel(SoundEngineImpl& impl,
            hash_t sound_id,
            const SoundSystem::SoundDescriptor& descriptor,
            const math::vec3& position,
            const math::vec3& velocity,
            float volume_dB):
    impl(impl),
    channel(nullptr),
    state(State::INITIALIZING),
    position(position),
    velocity(velocity),
    volume_dB(volume_dB),
    sound_id(sound_id),
    should_stop(false),
    descriptor(descriptor)
    {

    }

    Channel(SoundEngineImpl& impl,
            const SoundSystem::SoundDescriptor& descriptor,
            const math::vec3& position,
            const math::vec3& velocity,
            float volume_dB):
    Channel(impl, H_(descriptor.filename.c_str()), descriptor, position, velocity, volume_dB)
    {

    }

    SoundEngineImpl& impl;
    FMOD::Channel*   channel;
    State            state;
    math::vec3       position;
    math::vec3       velocity;
    float            volume_dB;
    hash_t           sound_id;
    bool             should_stop;

    const SoundSystem::SoundDescriptor& descriptor;

    inline bool is_stopped() { return state == State::STOPPED; }
    inline bool is_playing() { return state == State::PLAYING; }

    void load_sound()
    {
        fs::path filepath = (descriptor.isfx ? SOUND_FX_PATH : SOUND_BGM_PATH) / descriptor.filename;

        DLOGI("load: <p>" + filepath.string() + "</p>", "sound", Severity::LOW);

        FMOD::Sound* out_sound = nullptr;
        FMOD_MODE mode = FMOD_DEFAULT;
        mode |= descriptor.is3d ? FMOD_3D : FMOD_2D;
        mode |= descriptor.loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
        mode |= descriptor.stream ? FMOD_CREATESTREAM : FMOD_CREATECOMPRESSEDSAMPLE;
        ERRCHECK(impl.fmodsys->createSound(filepath.string().c_str(), mode, 0, &out_sound));
        ERRCHECK(out_sound->set3DMinMaxDistance(descriptor.min_distance, descriptor.max_distance));

        impl.sounds.insert(std::pair(sound_id, out_sound));
    }

    bool prepare_play()
    {
        ERRCHECK(impl.fmodsys->playSound(impl.sounds.at(sound_id), 0, true, &channel));

        if(channel)
        {
            float volume = db_to_linear(volume_dB);
            #ifdef __DEBUG__
                std::stringstream ss;
                ss << "position: " << position;
                DLOGI(ss.str(), "sound", Severity::LOW);
                ss.str("");

                ss << "velocity: " << velocity;
                DLOGI(ss.str(), "sound", Severity::LOW);
                ss.str("");

                ss << "volume: " << volume_dB << "dB = " << volume;
                DLOGI(ss.str(), "sound", Severity::LOW);
                ss.str("");

                /*ss << "channel: " << channel_id;
                DLOGI(ss.str(), "sound", Severity::LOW);*/
            #endif

            FMOD_VECTOR pos = to_fmod_vec(position);
            FMOD_VECTOR vel = to_fmod_vec(velocity);
            ERRCHECK(channel->set3DAttributes(&pos, &vel));
            ERRCHECK(channel->setVolume(volume));
            ERRCHECK(channel->setPaused(false));

            return true;
        }

        return false;
    }

    void update(float dt)
    {
        switch(state)
        {
            case State::INITIALIZING:
                // Any randomization/adjustment of pitch/volume... goes here
                state = State::LOADING;
                [[fallthrough]];

            case State::LOADING:
            {
                // Load sound if not already loaded
                auto it = impl.sounds.find(sound_id);
                if(it == impl.sounds.end())
                    load_sound();
                state = State::PREPARING;
                [[fallthrough]];
            }

            case State::PREPARING:
            {
                state = prepare_play() ? State::PLAYING : State::STOPPING;
                return;
            }

            case State::PLAYING:
            {
                // Update channel parameters here

                bool is_playing = false;
                channel->isPlaying(&is_playing);
                if(!is_playing || should_stop)
                {
                    state = State::STOPPING;
                    return;
                }
                return;
            }

            case State::STOPPING:
            {
                // Update fadeout...

                channel->stop();
                state = State::STOPPED;
                return;
            }

            case State::STOPPED: break;
        }
    }
};

SoundSystem::SoundDescriptor::SoundDescriptor(const char* filename):
filename(filename),
volume_dB(0.f),
min_distance(0.5f),
max_distance(100.f),
loop(false),
stream(false),
is3d(true),
isfx(true)
{

}

SoundSystem::SoundSystem():
pimpl_(new SoundEngineImpl()),
distance_factor_(1.0f),
doppler_scale_(1.0f),
rolloff_scale_(1.0f),
last_campos_(0.f)
{
    DLOGS("[SoundSystem] Initializing.", "sound", Severity::LOW);
    DLOGN("Retrieving config data.", "sound", Severity::LOW);

    // * Retrieve config data
    // Mandatory stuff
    if(!CONFIG.get(H_("root.folders.soundfx"), SOUND_FX_PATH))
    {
        DLOGE("[SoundSystem] Cannot find config path 'soundfx'.", "sound", Severity::CRIT);
        fatal();
    }
    if(!CONFIG.get(H_("root.folders.soundbgm"), SOUND_BGM_PATH))
    {
        DLOGE("[SoundSystem] Cannot find config path 'soundbgm'.", "sound", Severity::CRIT);
        fatal();
    }
    if(!fs::exists(SOUND_FX_PATH))
    {
        DLOGE("[SoundSystem] Cannot find 'soundfx' folder.", "sound", Severity::CRIT);
        fatal();
    }
    if(!fs::exists(SOUND_BGM_PATH))
    {
        DLOGE("[SoundSystem] Cannot find 'soundbgm' folder.", "sound", Severity::CRIT);
        fatal();
    }

    // Optional stuff
    uint32_t max_channels = 128;
    CONFIG.get(H_("root.sound.general.max_channels"), max_channels);
    CONFIG.get(H_("root.sound.general.distance_factor"), distance_factor_);
    CONFIG.get(H_("root.sound.general.doppler_scale"), doppler_scale_);
    CONFIG.get(H_("root.sound.general.rolloff_scale"), rolloff_scale_);

    // * Initialize FMOD
    DLOGN("Initializing FMOD.", "sound", Severity::LOW);
    void* extradriverdata = nullptr;
    unsigned int version;
    ERRCHECK(FMOD::System_Create(&pimpl_->fmodsys));
    ERRCHECK(pimpl_->fmodsys->getVersion(&version));
    assert(version < FMOD_VERSION && "FMOD header/lib version mismatch.");
    ERRCHECK(pimpl_->fmodsys->init(max_channels, FMOD_INIT_3D_RIGHTHANDED, extradriverdata));
    ERRCHECK(pimpl_->fmodsys->set3DSettings(doppler_scale_, distance_factor_, rolloff_scale_));

    // TMP test
    register_sound(SoundDescriptor("swish.wav"));
    //load_sound("swish.wav"_h);

    DLOGES("sound", Severity::LOW);
}

SoundSystem::~SoundSystem()
{
    for(auto&& [key,psound]: pimpl_->sounds)
        ERRCHECK(psound->release());

    ERRCHECK(pimpl_->fmodsys->close());
    ERRCHECK(pimpl_->fmodsys->release());
}

void SoundSystem::update(const GameClock& clock)
{
    // * First frame -> frame duration = 0.f -> do nothing
    float dt = clock.get_frame_duration();
    if(dt>0.f)
    {
        auto it = pimpl_->channels.begin();
        while(it != pimpl_->channels.end())
        {
            // Remove dead channels
            if(it->second->is_stopped())
                pimpl_->channels.erase(it++);
            else
            {
                it->second->update(dt);
                ++it;
            }
        }

        // * Get camera position / axes and update listener
        auto pscene = locate<Scene>(H_("Scene"));
        auto cam = pscene->get_camera();

        const math::vec3& campos = cam->get_position();

        math::vec3 camvel((campos-last_campos_) * 1.f/clock.get_frame_duration());
        last_campos_ = campos;

        FMOD_VECTOR listenerpos = to_fmod_vec(campos);
        FMOD_VECTOR listenervel = to_fmod_vec(camvel);
        FMOD_VECTOR forward     = to_fmod_vec(cam->get_forward());
        FMOD_VECTOR up          = to_fmod_vec(cam->get_up());
        ERRCHECK(pimpl_->fmodsys->set3DListenerAttributes(0, &listenerpos, &listenervel, &forward, &up));
    }
}

void SoundSystem::init_events(InputHandler& handler)
{

}

#ifndef __DISABLE_EDITOR__
void SoundSystem::generate_widget()
{
    ImGui::SetNextTreeNodeOpen(false, ImGuiCond_Once);
    if(ImGui::CollapsingHeader("Sound System"))
    {
        if(ImGui::Button("Test 3D sound"))
        {
            play_sound("swish.wav"_h, math::vec3(0), math::vec3(0));
        }
    }
}
#endif

void SoundSystem::register_sound(const SoundDescriptor& descriptor, hash_t name)
{
    fs::path filepath = SOUND_FX_PATH / descriptor.filename;
    if(!fs::exists(filepath))
    {
        DLOGE("[SoundSystem] Cannot find sound fx: ", "sound", Severity::CRIT);
        DLOGI(filepath.string(), "sound", Severity::CRIT);
        return;
    }

    if(name == 0)
        name = H_(descriptor.filename.c_str());

    if(descriptors_.find(name) != descriptors_.end())
    {
        DLOGE("[SoundSystem] Already loaded descriptor (or possible collision): ", "sound", Severity::WARN);
        DLOGI(filepath.string(), "sound", Severity::WARN);
        DLOGI("Skipping.", "sound", Severity::WARN);
        return;
    }

    DLOGN("[SoundSystem] Loading descriptor: ", "sound", Severity::LOW);
    DLOGI(filepath.string(), "sound", Severity::LOW);

    descriptors_.insert(std::pair(name, descriptor));
    // Multiply min_dist & max_dist by distance_factor here if needed
}

bool SoundSystem::load_sound(hash_t name)
{
    auto it = descriptors_.find(name);
    if(it == descriptors_.end())
    {
        DLOGE("[SoundSystem] Cannot find sound descriptor: ", "sound", Severity::WARN);
        DLOGI(std::to_string(name) + " -> " + HRESOLVE(name), "sound", Severity::WARN);
        DLOGI("Skipping.", "sound", Severity::WARN);
        return false;
    }

    auto& desc = it->second;
    fs::path filepath = (desc.isfx ? SOUND_FX_PATH : SOUND_BGM_PATH) / desc.filename;

    DLOGN("[SoundSystem] Loading sound: ", "sound", Severity::LOW);
    DLOGI(filepath.string(), "sound", Severity::LOW);

    FMOD::Sound* out_sound = nullptr;
    FMOD_MODE mode = FMOD_DEFAULT;
    mode |= desc.is3d ? FMOD_3D : FMOD_2D;
    mode |= desc.loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
    mode |= desc.stream ? FMOD_CREATESTREAM : FMOD_CREATECOMPRESSEDSAMPLE;
    ERRCHECK(pimpl_->fmodsys->createSound(filepath.string().c_str(), mode, 0, &out_sound));
    ERRCHECK(out_sound->set3DMinMaxDistance(desc.min_distance * distance_factor_, desc.max_distance * distance_factor_));

    pimpl_->sounds.insert(std::pair(name, out_sound));
    return true;
}


bool SoundSystem::unload_sound(hash_t name)
{
    auto it = pimpl_->sounds.find(name);
    if(it == pimpl_->sounds.end())
    {
        DLOGE("[SoundSystem] Cannot unload unknown sounds: ", "sound", Severity::WARN);
        DLOGI(std::to_string(name) + " -> " + HRESOLVE(name), "sound", Severity::WARN);
        return false;
    }
    pimpl_->sounds.erase(it);
    return true;
}

int SoundSystem::play_sound(hash_t name,
                            const math::vec3& position,
                            const math::vec3& velocity,
                            float volume_dB)
{
    DLOGN("Playing sounds: " + std::to_string(name) + " -> <n>" + HRESOLVE(name) + "</n>", "sound", Severity::LOW);

    int channel_id = pimpl_->next_channel_id++;
    auto it = descriptors_.find(name);
    if(it != descriptors_.end())
    {
        pimpl_->channels[channel_id] = std::make_unique<SoundEngineImpl::Channel>
        (
            *pimpl_,
            name,
            it->second,
            position,
            velocity,
            volume_dB
        );
    }
    return channel_id;
}

} // namespace wcore
