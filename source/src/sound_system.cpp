#include <map>
#include <sstream>
#include <filesystem>

#include "scene.h"
#include "algorithms.h"
#include "sound_system.h"
#include "xml_utils.hpp"
#include "io_utils.h"
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

static fs::path get_sound_path(const SoundSystem::SoundDescriptor& desc)
{
    fs::path filepath;
    switch(desc.sound_type)
    {
        case SoundSystem::SoundDescriptor::SoundType::FX:
            filepath = SOUND_FX_PATH;
            break;

        case SoundSystem::SoundDescriptor::SoundType::BGM:
            filepath = SOUND_BGM_PATH;
            break;
    }
    return filepath / desc.filename;
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
    channel_group_bgm(nullptr),
    channel_group_fx(nullptr),
    channel_group_master(nullptr),
    next_channel_id(1),
    channel_fadeout_s(0.1f)
    {

    }

    typedef std::map<hash_t, FMOD::Sound*> SoundMap;
    typedef std::map<int, std::unique_ptr<Channel>> ChannelMap;

    FMOD::System* fmodsys;
    FMOD::ChannelGroup* channel_group_bgm;
    FMOD::ChannelGroup* channel_group_fx;
    FMOD::ChannelGroup* channel_group_master;

    SoundMap      sounds;
    ChannelMap    channels;
    int           next_channel_id;
    float         channel_fadeout_s;
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

    SoundEngineImpl& impl;
    FMOD::Channel*   channel;
    FMOD::DSP*       lowpass;
    State            state;
    math::vec3       position;
    math::vec3       velocity;
    float            volume_dB;
    float            fadeout_accum;
    hash_t           sound_id;
    bool             should_stop;
    bool             dist_filter;

public:
    Channel(SoundEngineImpl& impl,
            hash_t sound_id,
            const SoundSystem::SoundDescriptor& descriptor,
            const math::vec3& position,
            const math::vec3& velocity,
            float volume_dB,
            bool dist_filter=false):
    impl(impl),
    channel(nullptr),
    lowpass(nullptr),
    state(State::INITIALIZING),
    position(position),
    velocity(velocity),
    volume_dB(volume_dB + descriptor.volume_dB),
    fadeout_accum(0.f),
    sound_id(sound_id),
    should_stop(false),
    dist_filter(dist_filter),
    descriptor(descriptor)
    {

    }

    Channel(SoundEngineImpl& impl,
            const SoundSystem::SoundDescriptor& descriptor,
            const math::vec3& position,
            const math::vec3& velocity,
            float volume_dB,
            bool dist_filter=false):
    Channel(impl, H_(descriptor.filename.c_str()), descriptor, position, velocity, volume_dB, dist_filter)
    {

    }

    const SoundSystem::SoundDescriptor& descriptor;

    inline bool is_stopped() { return state == State::STOPPED; }
    inline bool is_playing() { return state == State::PLAYING; }
    inline void stop()       { should_stop = true; }
    inline void mute(bool value)  { ERRCHECK(channel->setMute(value)); }
    inline void set_channel_bgm() { ERRCHECK(channel->setChannelGroup(impl.channel_group_bgm)); }
    inline void set_channel_fx()  { ERRCHECK(channel->setChannelGroup(impl.channel_group_fx)); }

    void update(float dt, const math::vec3& campos);

private:
    void update_channel_parameters();
    void update_filter(const math::vec3& campos);
    void load_sound();
    bool prepare_play();
};

void SoundSystem::SoundEngineImpl::Channel::update(float dt, const math::vec3& campos)
{
    switch(state)
    {
        case State::INITIALIZING:
        {
            DLOGN("New sound channel.", "sound", Severity::LOW);
            DLOGI("sound id: " + std::to_string(sound_id) + " -> <n>" + HRESOLVE(sound_id) + "</n>", "sound", Severity::LOW);
            // Any randomization/adjustment of pitch/volume... goes here
            state = State::LOADING;
            [[fallthrough]];
        }

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
            state = prepare_play() ? State::PLAYING : State::STOPPED;
            return;
        }

        case State::PLAYING:
        {
            if(dist_filter)
                update_filter(campos);
            update_channel_parameters();

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
            // Update fadeout
            bool is_playing = false;
            channel->isPlaying(&is_playing);
            if(is_playing)
            {
                // Update volume linearly during fadeout phase
                fadeout_accum += dt;
                float alpha = 1.f - std::min(1.f, fadeout_accum / impl.channel_fadeout_s);
                float volume = alpha * db_to_linear(volume_dB);
                ERRCHECK(channel->setVolume(volume));

                // Stop channel at the end of fadeout
                if(fadeout_accum>=impl.channel_fadeout_s)
                {
                    channel->stop();
                    state = State::STOPPED;
                }
            }
            else
                state = State::STOPPED;
            return;
        }

        case State::STOPPED: break;
    }
}

void SoundSystem::SoundEngineImpl::Channel::update_channel_parameters()
{
    if(descriptor.is3d)
    {
        FMOD_VECTOR pos = to_fmod_vec(position);
        FMOD_VECTOR vel = to_fmod_vec(velocity);
        ERRCHECK(channel->set3DAttributes(&pos, &vel));
    }

    switch(descriptor.sound_type)
    {
        case SoundDescriptor::SoundType::FX:
            set_channel_fx();
            break;

        case SoundDescriptor::SoundType::BGM:
            set_channel_bgm();
            break;
    }

    float volume = db_to_linear(volume_dB);
    ERRCHECK(channel->setVolume(volume));
}

void SoundSystem::SoundEngineImpl::Channel::update_filter(const math::vec3& campos)
{
    // Filter sound according to emitter distance
    float dist = (position-campos).norm();
    float cutoff = 1000.f + 19000.f * (1.f-std::min(1.f, dist / descriptor.max_distance));
    ERRCHECK(lowpass->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF, cutoff));
}

void SoundSystem::SoundEngineImpl::Channel::load_sound()
{
    fs::path filepath = get_sound_path(descriptor);

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

bool SoundSystem::SoundEngineImpl::Channel::prepare_play()
{
    ERRCHECK(impl.fmodsys->playSound(impl.sounds.at(sound_id), 0, true, &channel));

    if(channel)
    {
        #ifdef __DEBUG__
            std::stringstream ss;
            ss << "streamed: " << (descriptor.stream?"true":"false");
            DLOGI(ss.str(), "sound", Severity::DET);
            ss.str("");

            ss << "position: " << position;
            DLOGI(ss.str(), "sound", Severity::DET);
            ss.str("");

            ss << "velocity: " << velocity;
            DLOGI(ss.str(), "sound", Severity::DET);
            ss.str("");

            ss << "volume: " << volume_dB << "dB";
            DLOGI(ss.str(), "sound", Severity::DET);
            ss.str("");
        #endif

        update_channel_parameters();
        ERRCHECK(channel->setPaused(false));

        if(dist_filter)
        {
            ERRCHECK(impl.fmodsys->createDSPByType(FMOD_DSP_TYPE_LOWPASS, &lowpass));
            ERRCHECK(lowpass->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF, 20000.0f));
            ERRCHECK(channel->addDSP(0, lowpass));
        }

        return true;
    }

    return false;
}

SoundSystem::SoundDescriptor::SoundDescriptor(const std::string& filename):
filename(filename),
volume_dB(0.f),
min_distance(0.5f),
max_distance(100.f),
loop(false),
stream(false),
is3d(true),
sound_type(SoundType::FX)
{

}

SoundSystem::SoundSystem():
pimpl_(new SoundEngineImpl()),
distance_factor_(1.0f),
doppler_scale_(1.0f),
rolloff_scale_(1.0f),
vol_fx_(100.f),
vol_bgm_(100.f),
vol_master_(100.f),
mute_(false),
last_campos_(0.f)
{
    DLOGS("[SoundSystem] Initializing.", "sound", Severity::LOW);
    DLOGN("Retrieving config data.", "sound", Severity::LOW);

    // * Retrieve config data
    // Mandatory stuff
    if(!CONFIG.get("root.folders.soundfx"_h, SOUND_FX_PATH))
    {
        DLOGE("[SoundSystem] Cannot find config path 'soundfx'.", "sound", Severity::CRIT);
        fatal();
    }
    if(!CONFIG.get("root.folders.soundbgm"_h, SOUND_BGM_PATH))
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
    CONFIG.get("root.sound.general.mute"_h,              mute_);
    CONFIG.get("root.sound.general.volume_fx"_h,         vol_fx_);
    CONFIG.get("root.sound.general.volume_bgm"_h,        vol_bgm_);
    CONFIG.get("root.sound.general.volume_master"_h,     vol_master_);
    CONFIG.get("root.sound.general.max_channels"_h,      max_channels);
    CONFIG.get("root.sound.general.distance_factor"_h,   distance_factor_);
    CONFIG.get("root.sound.general.doppler_scale"_h,     doppler_scale_);
    CONFIG.get("root.sound.general.rolloff_scale"_h,     rolloff_scale_);
    CONFIG.get("root.sound.general.channel_fadeout_s"_h, pimpl_->channel_fadeout_s);

    // Sanity check
    vol_fx_     = math::clamp(vol_fx_, 0.f, 100.f);
    vol_bgm_    = math::clamp(vol_bgm_, 0.f, 100.f);
    vol_master_ = math::clamp(vol_master_, 0.f, 100.f);

    // * Initialize FMOD
    DLOGN("Initializing FMOD.", "sound", Severity::LOW);
    void* extradriverdata = nullptr;
    unsigned int version;
    ERRCHECK(FMOD::System_Create(&pimpl_->fmodsys));
    ERRCHECK(pimpl_->fmodsys->getVersion(&version));
    assert(version < FMOD_VERSION && "FMOD header/lib version mismatch.");
    ERRCHECK(pimpl_->fmodsys->init(max_channels, FMOD_INIT_3D_RIGHTHANDED, extradriverdata));
    ERRCHECK(pimpl_->fmodsys->set3DSettings(doppler_scale_, distance_factor_, rolloff_scale_));

    // * Channel groups
    ERRCHECK(pimpl_->fmodsys->createChannelGroup("fx",  &pimpl_->channel_group_fx));
    ERRCHECK(pimpl_->fmodsys->createChannelGroup("bgm", &pimpl_->channel_group_bgm));
    ERRCHECK(pimpl_->fmodsys->getMasterChannelGroup(&pimpl_->channel_group_master));
    ERRCHECK(pimpl_->channel_group_master->addGroup(pimpl_->channel_group_fx));
    ERRCHECK(pimpl_->channel_group_master->addGroup(pimpl_->channel_group_bgm));
    ERRCHECK(pimpl_->channel_group_fx->setVolume(vol_fx_/100.f));
    ERRCHECK(pimpl_->channel_group_bgm->setVolume(vol_bgm_/100.f));
    ERRCHECK(pimpl_->channel_group_master->setVolume(vol_master_/100.f));

    // * Parse sound descriptions
    parse_asset_file("sounds.xml");

    DLOGES("sound", Severity::LOW);
}

SoundSystem::~SoundSystem()
{
    for(auto&& [key,psound]: pimpl_->sounds)
        ERRCHECK(psound->release());

    ERRCHECK(pimpl_->fmodsys->close());
    ERRCHECK(pimpl_->fmodsys->release());
}

void SoundSystem::parse_asset_file(const char* xmlfile)
{
    fs::path file_path(io::get_file("root.folders.level"_h, xmlfile));
    xml_parser_.load_file_xml(file_path);

    for (rapidxml::xml_node<>* sound_node=xml_parser_.get_root()->first_node("Sound");
         sound_node;
         sound_node=sound_node->next_sibling("Sound"))
    {
        std::string snd_type, snd_name, snd_location;
        if(xml::parse_attribute(sound_node, "location", snd_location))
        {
            bool has_name = xml::parse_attribute(sound_node, "name", snd_name);
            hash_t hname = has_name ? H_(snd_name.c_str()) : 0;

            SoundDescriptor desc(snd_location);

            if(xml::parse_attribute(sound_node, "type", snd_type))
            switch(H_(snd_type.c_str()))
            {
                case "fx"_h:
                    desc.sound_type = SoundDescriptor::SoundType::FX;
                    break;

                case "bgm"_h:
                    desc.sound_type = SoundDescriptor::SoundType::BGM;
                    desc.is3d = false;
                    desc.stream = true;
                    break;
            }

            xml::parse_node(sound_node, "Volume",      desc.volume_dB);
            xml::parse_node(sound_node, "MinDistance", desc.min_distance);
            xml::parse_node(sound_node, "MaxDistance", desc.max_distance);
            xml::parse_node(sound_node, "Loop",        desc.loop);
            xml::parse_node(sound_node, "Stream",      desc.stream);

            register_sound(desc, hname);
#ifdef __DEBUG__
            HRESOLVE.add_intern_string(has_name ? snd_name : snd_location);
#endif
        }
    }
}

void SoundSystem::update(const GameClock& clock)
{
    // * First frame -> frame duration = 0.f -> do nothing
    float dt = clock.get_frame_duration();
    if(dt>0.f)
    {
        // * Get camera position / axes and update listener
        auto pscene = locate<Scene>("Scene"_h);
        auto cam = pscene->get_camera();

        const math::vec3& campos = cam->get_position();


        auto it = pimpl_->channels.begin();
        while(it != pimpl_->channels.end())
        {
            // Remove dead channels
            if(it->second->is_stopped())
                pimpl_->channels.erase(it++);
            else
            {
                it->second->update(dt, campos);
                ++it;
            }
        }

        math::vec3 camvel((campos-last_campos_) * 1.f/clock.get_frame_duration());
        last_campos_ = campos;

        FMOD_VECTOR listenerpos = to_fmod_vec(campos);
        FMOD_VECTOR listenervel = to_fmod_vec(camvel);
        FMOD_VECTOR forward     = to_fmod_vec(cam->get_forward());
        FMOD_VECTOR up          = to_fmod_vec(cam->get_up());
        ERRCHECK(pimpl_->fmodsys->set3DListenerAttributes(0, &listenerpos, &listenervel, &forward, &up));
        ERRCHECK(pimpl_->fmodsys->update());
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
        if(ImGui::Checkbox("Mute", &mute_))
            for(auto&& [key,channel]: pimpl_->channels)
                channel->mute(mute_);

        if(ImGui::SliderFloat("Master volume", &vol_master_, 0.0f, 100.0f))
            pimpl_->channel_group_master->setVolume(vol_master_/100.f);

        if(ImGui::SliderFloat("FX volume", &vol_fx_, 0.0f, 100.0f))
            pimpl_->channel_group_fx->setVolume(vol_fx_/100.f);

        if(ImGui::SliderFloat("BGM volume", &vol_bgm_, 0.0f, 100.0f))
            pimpl_->channel_group_bgm->setVolume(vol_bgm_/100.f);

        if(ImGui::Button("Swish sound"))
            play_sound("swish"_h, math::vec3(0), math::vec3(0));

        if(ImGui::Button("Loop sound"))
            play_sound("drumloop"_h, math::vec3(9.f,2.f,17.f), math::vec3(0), 0.f, true);

        if(ImGui::Button("music"))
            play_bgm("maze_bgm"_h);

        for(auto&& [key,channel]: pimpl_->channels)
            if(ImGui::Button(("Stop chan #" + std::to_string(key)).c_str()))
                channel->stop();
    }
}
#endif

void SoundSystem::register_sound(const SoundDescriptor& descriptor, hash_t name)
{
    fs::path filepath = get_sound_path(descriptor);
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
    DLOGI("<p>" + filepath.string() + "</p>", "sound", Severity::LOW);

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
    fs::path filepath = get_sound_path(desc);

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
                            float volume_dB,
                            bool dist_filter)
{
    if(mute_) return 0;

    DLOGN("Playing sound: " + std::to_string(name) + " -> <n>" + HRESOLVE(name) + "</n>", "sound", Severity::LOW);

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
            volume_dB,
            dist_filter
        );
        DLOGI("Channel id: <v>" + std::to_string(channel_id) + "</v>", "sound", Severity::DET);
    }
    else
    {
        DLOGW("Unable to find sound: " + std::to_string(name) + " -> <n>" + HRESOLVE(name) + "</n>", "sound", Severity::WARN);
        DLOGI("Skipping.", "sound", Severity::WARN);
    }
    return channel_id;
}

int SoundSystem::play_bgm(hash_t name, float volume_dB)
{
    if(mute_) return 0;

    DLOGN("Playing background music: " + std::to_string(name) + " -> <n>" + HRESOLVE(name) + "</n>", "sound", Severity::LOW);

    int channel_id = pimpl_->next_channel_id++;
    auto it = descriptors_.find(name);
    if(it != descriptors_.end())
    {
        pimpl_->channels[channel_id] = std::make_unique<SoundEngineImpl::Channel>
        (
            *pimpl_,
            name,
            it->second,
            math::vec3(0),
            math::vec3(0),
            volume_dB
        );
        DLOGI("Channel id: <v>" + std::to_string(channel_id) + "</v>", "sound", Severity::DET);
    }
    else
    {
        DLOGW("Unable to find background music: " + std::to_string(name) + " -> <n>" + HRESOLVE(name) + "</n>", "sound", Severity::WARN);
        DLOGI("Skipping.", "sound", Severity::WARN);
    }
    return channel_id;
}

bool SoundSystem::set_channel_position(int channel_id, const math::vec3& position)
{
    auto it = pimpl_->channels.find(channel_id);
    if(it == pimpl_->channels.end())
        return false;

    it->second->position = position;
    return true;
}

bool SoundSystem::set_channel_velocity(int channel_id, const math::vec3& velocity)
{
    auto it = pimpl_->channels.find(channel_id);
    if(it == pimpl_->channels.end())
        return false;

    it->second->velocity = velocity;
    return true;
}

bool SoundSystem::set_channel_volume(int channel_id, float volume)
{
    auto it = pimpl_->channels.find(channel_id);
    if(it == pimpl_->channels.end())
        return false;

    it->second->volume_dB = volume;
    return true;
}

bool SoundSystem::stop_channel(int channel_id)
{
    auto it = pimpl_->channels.find(channel_id);
    if(it == pimpl_->channels.end())
        return false;

    it->second->should_stop = true;
    return true;
}

} // namespace wcore
