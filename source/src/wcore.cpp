#include <map>
#include <cassert>
#include <fstream>

#include "wcore.h"

#include "gfx_api.h" // Won't compile if removed: ensures GLFW header included before GL
#include "thread_utils.h"
#include "error.h"
#include "config.h"
#include "intern_string.h"
#include "engine_core.h"
#include "scene.h"
#include "file_system.h"
#include "chunk_manager.h"
#include "camera_controller.h"
#include "scene_loader.h"
#include "game_object_factory.h"
#include "input_handler.h"
#include "entity_system.h"
#include "sound_system.h"
#include "pipeline.h"
#include "daylight.h"
#include "ray_caster.h"
#include "debug_info.h"
#ifndef __DISABLE_EDITOR__
    #include "editor.h"
    #include "editor_tweaks.h"
#endif
#include "globals.h"
#include "logger.h"

namespace wcore
{

static void warn_global_not_found(hash_t name)
{
    DLOGW("Global name not found:", "core");
    DLOGI(std::to_string(name), "core");
    DLOGW("Skipping.", "core");
}

void SetGlobal(hash_t name, const void* data)
{
    switch(name)
    {
        default:
            warn_global_not_found(name);
            break;
        case "SCR_W"_h:
            GLB.SCR_W = *reinterpret_cast<const uint32_t*>(data);
            break;
        case "SCR_H"_h:
            GLB.SCR_H = *reinterpret_cast<const uint32_t*>(data);
            break;
        case "SCR_FULL"_h:
            GLB.SCR_FULL = *reinterpret_cast<const bool*>(data);
            break;
        case "START_LEVEL"_h:
            char* value = const_cast<char*>(reinterpret_cast<const char*>(data));
            GLB.START_LEVEL = value;
            break;
    }
}

//    ______ _                 _
//    | ___ (_)               | |
//    | |_/ /_ _ __ ___  _ __ | |
//    |  __/| | '_ ` _ \| '_ \| |
//    | |   | | | | | | | |_) | |
//    \_|   |_|_| |_| |_| .__/|_|
//                      | |
//                      |_|

struct Engine::EngineImpl
{
    EngineImpl():
    engine_core(nullptr),
#ifndef __DISABLE_EDITOR__
    ed_tweaks(nullptr),
#endif
    scene(nullptr),
    entity_system(nullptr),
    camera_controller(nullptr),
    game_object_factory(nullptr),
    scene_loader(nullptr),
    pipeline(nullptr),
    daylight(nullptr),
    ray_caster(nullptr),
    chunk_manager(nullptr),
    sound_system(nullptr)
#ifndef __DISABLE_EDITOR__
    ,editor(nullptr)
#endif
    {
        // Instanciate singletons
        FileSystem::Instance();
#ifdef __DEBUG__
        InternStringLocator::Instance();
#endif
    }

    ~EngineImpl()
    {
#ifndef __DISABLE_EDITOR__
        delete ed_tweaks;
        delete editor;
#endif
        delete sound_system;
        delete chunk_manager;
        delete ray_caster;
        delete daylight;
        delete pipeline;
        delete scene_loader;
        delete game_object_factory;
        delete camera_controller;
        delete entity_system;
        delete scene;
        delete engine_core;

        // Kill singletons
        DebugInfo::Kill();
#ifdef __DEBUG__
        InternStringLocator::Kill();
#endif
        FileSystem::Kill();
        Config::Kill();
        Logger::Kill();
    }

    void init(AbstractContext* context=nullptr)
    {
        engine_core         = new EngineCore(context);
        DLOG("EngineCore <g>created</g>", "core", Severity::LOW);

#ifndef __DISABLE_EDITOR__
        ed_tweaks           = new EditorTweaksInitializer();
        DLOG("EditorTweaksInitializer <g>created</g>", "core", Severity::LOW);
#endif

        scene               = new Scene();
        DLOG("Scene <g>created</g>", "core", Severity::LOW);

        entity_system       = new EntitySystem();
        DLOG("EntitySystem <g>created</g>", "core", Severity::LOW);

        camera_controller   = new CameraController();
        DLOG("CameraController <g>created</g>", "core", Severity::LOW);

        game_object_factory = new GameObjectFactory();
        DLOG("GameObjectFactory <g>created</g>", "core", Severity::LOW);

        scene_loader        = new SceneLoader();
        DLOG("SceneLoader <g>created</g>", "core", Severity::LOW);

        pipeline            = new RenderPipeline();
        DLOG("RenderPipeline <g>created</g>", "core", Severity::LOW);

        daylight            = new DaylightSystem();
        DLOG("DaylightSystem <g>created</g>", "core", Severity::LOW);

        ray_caster          = new RayCaster();
        DLOG("RayCaster <g>created</g>", "core", Severity::LOW);

        chunk_manager       = new ChunkManager();
        DLOG("ChunkManager <g>created</g>", "core", Severity::LOW);

        sound_system        = new SoundSystem();
        DLOG("SoundSystem <g>created</g>", "core", Severity::LOW);

#ifndef __DISABLE_EDITOR__
        editor              = new Editor();
        DLOG("Editor <g>created</g>", "core", Severity::LOW);
#endif

    }

    EngineCore* engine_core;

    // InitializerSystems
#ifndef __DISABLE_EDITOR__
    EditorTweaksInitializer* ed_tweaks;
#endif

    // GameSystems
    Scene*             scene;
    EntitySystem*      entity_system;
    CameraController*  camera_controller;
    GameObjectFactory* game_object_factory;
    SceneLoader*       scene_loader;
    RenderPipeline*    pipeline;
    DaylightSystem*    daylight;
    RayCaster*         ray_caster;
    ChunkManager*      chunk_manager;
    SoundSystem*       sound_system;
#ifndef __DISABLE_EDITOR__
    Editor*            editor;
#endif
};

//      ___  ______ _____  ______
//     / _ \ | ___ \_   _| | ___ \
//    / /_\ \| |_/ / | |   | |_/ / __ _ ___  ___
//    |  _  ||  __/  | |   | ___ \/ _` / __|/ _ \
//    | | | || |    _| |_  | |_/ / (_| \__ \  __/
//    \_| |_/\_|    \___/  \____/ \__,_|___/\___|


Engine::Engine()
{
    //thread::max_thread_priority();

    DLOG("<s>--- WCore: Loading config ---</s>", "core", Severity::LOW);
    // Parse main config file
    try
    {
        CONFIG.init();
    }
    catch (const std::ifstream::failure& e)
    {
        DLOGE("[Engine] Stream exception while initializing CONFIG singleton.", "core");
        DLOGI(e.what(), "core");
    }

    // Parse intern string hash table file
#ifdef __DEBUG__
    DLOG("<s>--- WCore: Initializing intern strings ---</s>", "core", Severity::LOW);
    try
    {
        HRESOLVE.init();
    }
    catch (const std::ifstream::failure& e)
    {
        DLOGE("[Engine] Stream exception while initializing intern string solver HRESOLVE.", "core");
        DLOGI(e.what(), "core");
    }
#endif

    eimpl_ = std::shared_ptr<EngineImpl>(new EngineImpl);
    scene = new SceneControl(eimpl_);
    pipeline = new PipelineControl(eimpl_);
}

Engine::~Engine()
{
    delete scene;
    delete pipeline;
}

void Engine::SetFrameSize(uint32_t width, uint32_t height, bool fullscreen)
{
    GLB.SCR_W = width;
    GLB.SCR_H = height;
    GLB.WIN_W = width;
    GLB.WIN_H = height;
    GLB.SCR_FULL = fullscreen;
}

void Engine::SetWindowSize(uint32_t width, uint32_t height)
{
    GLB.WIN_W = width;
    GLB.WIN_H = height;
}

bool Engine::UseResourceArchive(const char* filename, hash_t key)
{
    fs::path filepath;
    if(!CONFIG.get("root.folders.res"_h, filepath))
    {
        DLOGE("[Engine] Config node folders.res is not set.", "core");
        return false;
    }

    filepath /= filename;
    if(!fs::exists(filepath))
    {
        DLOGE("[Engine] Cannot open esource archive:", "core");
        DLOGI("<p>" + filepath.string() + "</p>", "core");
        return false;
    }

    DLOGN("[Engine] Opening resource archive: ", "core");
    DLOGI("<p>" + filepath.string() + "</p>", "core");
    FILESYSTEM.open_archive(filepath, key);
    return true;
}

void Engine::Init(int argc, char const *argv[],
                  void(*parse_arguments)(int, char const **),
                  AbstractContext* context)
{
    DLOG("<s>--- WCore: Parsing program arguments ---</s>", "core", Severity::LOW);
    // First, try to initialize default values using config
    CONFIG.get("root.display.width"_h,  GLB.SCR_W);
    CONFIG.get("root.display.height"_h, GLB.SCR_H);
    CONFIG.get("root.display.full"_h,   GLB.SCR_FULL);

    // Parse command line arguments
    if(parse_arguments)
        parse_arguments(argc, argv);

    DLOG("<s>--- WCore: Loading default resource archive ---</s>", "core", Severity::LOW);
    UseResourceArchive("pack0.zip", "pack0"_h);

    DLOG("<s>--- WCore: Creating game systems ---</s>", "core", Severity::LOW);
    // Create game systems
    eimpl_->init(context);

    // * Initialize engine_core
    DLOG("<s>--- WCore: Bootstrapping initializer systems ---</s>", "core", Severity::LOW);
    // Render callbacks
    eimpl_->engine_core->set_render_func([&]()     { eimpl_->pipeline->render(); });
    eimpl_->engine_core->set_render_gui_func([&]() { eimpl_->pipeline->render_gui(); });
    // Initializer systems
#ifndef __DISABLE_EDITOR__
    eimpl_->engine_core->register_initializer_system("EditorTweaks"_h, static_cast<InitializerSystem*>(eimpl_->ed_tweaks));
#endif

    // Initialization step
    try
    {
        eimpl_->engine_core->init_system_parameters();
    }
    catch (const std::ifstream::failure& e)
    {
        DLOGE("[Engine] Stream exception while initializing system parameters.", "core");
        DLOGI(e.what(), "core");
        DLOGI("Initializer systems may use default state.", "core");
    }

    // * Register game systems (init events, register editor widgets, add to update list)
    DLOG("<s>--- WCore: Registering game systems ---</s>", "core", Severity::LOW);
#ifndef __DISABLE_EDITOR__
    eimpl_->engine_core->register_game_system("Editor"_h,            static_cast<GameSystem*>(eimpl_->editor));
#endif
    eimpl_->engine_core->register_game_system("CameraController"_h,  static_cast<GameSystem*>(eimpl_->camera_controller));
    eimpl_->engine_core->register_game_system("EntitySystem"_h,      static_cast<GameSystem*>(eimpl_->entity_system));
    eimpl_->engine_core->register_game_system("Scene"_h,             static_cast<GameSystem*>(eimpl_->scene));
    eimpl_->engine_core->register_game_system("Pipeline"_h,          static_cast<GameSystem*>(eimpl_->pipeline));
    eimpl_->engine_core->register_game_system("Daylight"_h,          static_cast<GameSystem*>(eimpl_->daylight));
    eimpl_->engine_core->register_game_system("RayCaster"_h,         static_cast<GameSystem*>(eimpl_->ray_caster));
    eimpl_->engine_core->register_game_system("GameObjectFactory"_h, static_cast<GameSystem*>(eimpl_->game_object_factory));
    eimpl_->engine_core->register_game_system("SceneLoader"_h,       static_cast<GameSystem*>(eimpl_->scene_loader));
    eimpl_->engine_core->register_game_system("ChunkManager"_h,      static_cast<GameSystem*>(eimpl_->chunk_manager));
    eimpl_->engine_core->register_game_system("SoundSystem"_h,       static_cast<GameSystem*>(eimpl_->sound_system));

    DLOG("<s>--- WCore: Initializing game systems ---</s>", "core", Severity::LOW);
    eimpl_->engine_core->init_game_systems();

    //auto&& input_handler = eimpl_->engine_core->get_input_handler();
    //dbg::LOG.track("input.mouse.locked"_h, input_handler);
    //dbg::LOG.track("input.mouse.unlocked"_h, input_handler);
    //dbg::LOG.track("input.mouse.focus"_h, input_handler);

    DLOG("<s>--- WCore: Done ---</s>", "core", Severity::LOW);
#ifdef __DEBUG__
    show_driver_error("post Init() glGetError(): ");
#endif
}

int Engine::Run()
{
    int ret = eimpl_->engine_core->run();
    DLOG("<s>--- WCore: Game loop stopped ---</s>", "core", Severity::LOW);
#ifdef __DEBUG__
    eimpl_->pipeline->dbg_show_statistics();
#endif
    DLOG("<s>--- WCore: Serializing ---</s>", "core", Severity::LOW);
    try
    {
        eimpl_->engine_core->serialize_system_parameters();
        eimpl_->engine_core->unload_game_systems();
    }
    catch (const std::ofstream::failure& e)
    {
        DLOGF("[Engine] Stream exception while serializing system parameters.", "core");
        DLOGI(e.what(), "core");
    }
    DLOG("<s>--- WCore: Meow! ---</s>", "core", Severity::LOW);
    return ret;
}

void Engine::Update(float dt)
{
    eimpl_->engine_core->handle_events();
    eimpl_->engine_core->update(dt);
}

void Engine::RenderFrame()
{
    eimpl_->engine_core->render();
}

void Engine::FinishFrame()
{
    eimpl_->engine_core->swap_buffers();
    eimpl_->engine_core->poll_events();
}

bool Engine::WindowRequired()
{
    return eimpl_->engine_core->window_required();
}


//     _____
//    /  ___|
//    \ `--.  ___ ___ _ __   ___
//     `--. \/ __/ _ \ '_ \ / _ \
//    /\__/ / (_|  __/ | | |  __/
//    \____/ \___\___|_| |_|\___|

Engine::SceneControl::SceneControl(std::shared_ptr<EngineImpl> impl):
eimpl_(impl)
{

}

void Engine::SceneControl::SetStartLevel(const char* level_name)
{
    GLB.START_LEVEL = level_name;
}

void Engine::SceneControl::LoadStart(const char* level_name)
{
    if(level_name != nullptr)
         GLB.START_LEVEL = level_name;

    // Load level
    eimpl_->scene_loader->load_level(GLB.START_LEVEL);
    eimpl_->scene_loader->load_global(*eimpl_->daylight);
    eimpl_->chunk_manager->load_start();

#ifdef __DEBUG__
    show_driver_error("post LoadStart() glGetError(): ");
#endif
}

void Engine::SceneControl::LoadLevel(const char* level_name)
{
    if(level_name != nullptr)
         GLB.START_LEVEL = level_name;

    // Load level but don't send geometry just yet
    eimpl_->scene_loader->load_level(GLB.START_LEVEL);
    eimpl_->scene_loader->load_global(*eimpl_->daylight);

#ifdef __DEBUG__
    show_driver_error("post LoadLevel() glGetError(): ");
#endif
}

uint32_t Engine::SceneControl::LoadChunk(uint32_t xx, uint32_t zz, bool send_geometry)
{
    return eimpl_->scene_loader->load_chunk(math::i32vec2(xx,zz), send_geometry);
}

void Engine::SceneControl::SendChunk(uint32_t xx, uint32_t zz)
{
    uint32_t chunk_index = std::hash<math::i32vec2>{}(math::i32vec2(xx,zz));
    eimpl_->scene->populate_static_octree(chunk_index);
    eimpl_->scene->load_geometry(chunk_index);
}

void Engine::SceneControl::LoadModel(hash_t name, uint32_t chunk_index, hash_t href)
{
    auto pmdl = eimpl_->scene_loader->load_model_instance(name, chunk_index, href);
}

void Engine::SceneControl::LoadPointLight(uint32_t chunk_index, hash_t href)
{
    auto plight = eimpl_->scene_loader->load_point_light(chunk_index, href);
}

Camera& Engine::SceneControl::GetCamera()
{
    return eimpl_->scene->get_camera();
}

Model& Engine::SceneControl::GetModelRef(hash_t href)
{
    return *eimpl_->scene->get_model_by_ref(href).lock();
}

Light& Engine::SceneControl::GetLightRef(hash_t href)
{
    return *eimpl_->scene->get_light_by_ref(href).lock();
}

Light& Engine::SceneControl::GetDirectionalLight()
{
    return *eimpl_->scene->get_directional_light_nc().lock();
}

bool Engine::SceneControl::VisitModelRef(hash_t href, std::function<void(Model& model)> visit)
{
    if(auto pmdl = eimpl_->scene->get_model_by_ref(href).lock())
    {
        visit(*pmdl);
        return true;
    }
    return false;
}

bool Engine::SceneControl::VisitLightRef(hash_t href, std::function<void(Light& light)> visit)
{
    if(auto plight = eimpl_->scene->get_light_by_ref(href).lock())
    {
        visit(*plight);
        return true;
    }
    return false;
}


//    ______ _            _ _
//    | ___ (_)          | (_)
//    | |_/ /_ _ __   ___| |_ _ __   ___
//    |  __/| | '_ \ / _ \ | | '_ \ / _ \
//    | |   | | |_) |  __/ | | | | |  __/
//    \_|   |_| .__/ \___|_|_|_| |_|\___|
//            | |
//            |_|

Engine::PipelineControl::PipelineControl(std::shared_ptr<EngineImpl> impl):
eimpl_(impl)
{

}

void Engine::PipelineControl::SetDefaultFrameBuffer(unsigned int index)
{
    Gfx::device->set_default_framebuffer(index);
}

void Engine::PipelineControl::SetShadowMappingEnabled(bool value)
{
    eimpl_->pipeline->set_shadow_mapping_enabled(value);
}

void Engine::PipelineControl::SetBloomEnabled(bool value)
{
    eimpl_->pipeline->set_bloom_enabled(value);
}

void Engine::PipelineControl::SetFogEnabled(bool value)
{
    eimpl_->pipeline->set_fog_enabled(value);
}

void Engine::PipelineControl::SetFXAAEnabled(bool value)
{
    eimpl_->pipeline->set_fxaa_enabled(value);
}

void Engine::PipelineControl::SetDaylightSystemEnabled(bool value)
{
    eimpl_->daylight->set_enabled(value);
}

void Engine::PipelineControl::SetLightingBrightThreshold(float value)
{
    eimpl_->pipeline->set_bright_threshold(value);
}

void Engine::PipelineControl::SetLightingBrightKnee(float value)
{
    eimpl_->pipeline->set_bright_knee(value);
}

void Engine::PipelineControl::SetShadowSlopeBias(float value)
{
    eimpl_->pipeline->set_shadow_slope_bias(value);
}

void Engine::PipelineControl::SetShadowBias(float value)
{
    eimpl_->pipeline->set_shadow_bias(value);
}

void Engine::PipelineControl::SetShadowNormalOffset(float value)
{
    eimpl_->pipeline->set_normal_offset(value);
}

void Engine::PipelineControl::SetDirectionalLightEnabled(bool value)
{
    eimpl_->pipeline->set_directional_light_enabled(value);
}


void Engine::PipelineControl::SetPostprocExposure(float value)
{
    eimpl_->pipeline->set_pp_exposure(value);
}

void Engine::PipelineControl::SetPostprocContrast(float value)
{
    eimpl_->pipeline->set_pp_contrast(value);
}

void Engine::PipelineControl::SetPostprocVibrance(float value)
{
    eimpl_->pipeline->set_pp_vibrance(value);
}

void Engine::PipelineControl::SetPostprocVignetteFalloff(float value)
{
    eimpl_->pipeline->set_pp_vignette_falloff(value);
}

void Engine::PipelineControl::SetPostprocVignetteBalance(float value)
{
    eimpl_->pipeline->set_pp_vignette_balance(value);
}

void Engine::PipelineControl::SetPostprocAberrationShift(float value)
{
    eimpl_->pipeline->set_pp_aberration_shift(value);
}

void Engine::PipelineControl::SetPostprocAberrationStrength(float value)
{
    eimpl_->pipeline->set_pp_aberration_strength(value);
}

void Engine::PipelineControl::SetPostprocAccBlindnessType(int value)
{
    eimpl_->pipeline->set_pp_acc_blindness_type(value);
}

void Engine::PipelineControl::SetPostprocVibranceBalance(const math::vec3& value)
{
    eimpl_->pipeline->set_pp_vibrance_balance(value);
}


#ifdef __DEBUG__
void Engine::PipelineControl::dShowLightProxy(int mode, float scale)
{
    eimpl_->pipeline->show_light_proxy(mode, scale);
}

void Engine::PipelineControl::dDrawSegment(const math::vec3& world_start,
                                           const math::vec3& world_end,
                                           int ttl,
                                           const math::vec3& color)
{
    eimpl_->pipeline->debug_draw_segment(world_start, world_end, ttl, color);
}

void Engine::PipelineControl::dDrawSphere(const math::vec3& world_pos,
                                          float radius,
                                          int ttl,
                                          const math::vec3& color)
{
    eimpl_->pipeline->debug_draw_sphere(world_pos, radius, ttl, color);
}

void Engine::PipelineControl::dDrawCross3(const math::vec3& world_pos,
                                          float radius,
                                          int ttl,
                                          const math::vec3& color)
{
    eimpl_->pipeline->debug_draw_cross3(world_pos, radius, ttl, color);
}

#endif

}
