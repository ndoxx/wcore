#include <map>
#include <fstream>

#include "wcore.h"

#include "gfx_driver.h" // Won't compile if removed: ensures GLFW header included before GL
#include "frame_buffer.h"
#include "thread_utils.h"
#include "error.h"
#include "config.h"
#include "intern_string.h"
#include "engine_core.h"
#include "scene.h"
#include "model.h"
#include "lights.h"
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
    DLOGW("Global name not found:", "core", Severity::WARN);
    DLOGI(std::to_string(name), "core", Severity::WARN);
    DLOGW("Skipping.", "core", Severity::WARN);
}

void GlobalsSet(hash_t name, const void* data)
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

struct Engine::EngineImpl
{
    EngineImpl():
    engine_core(nullptr),
    ed_tweaks(nullptr),
    scene(nullptr),
    entity_system(nullptr),
    camera_controller(nullptr),
    game_object_factory(nullptr),
    scene_loader(nullptr),
    pipeline(nullptr),
    daylight(nullptr),
    ray_caster(nullptr),
    chunk_manager(nullptr),
    sound_system(nullptr),
#ifndef __DISABLE_EDITOR__
    editor(nullptr),
#endif
    current_model_handle(0),
    current_light_handle(0)
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
#ifdef __DEBUG__
        InternStringLocator::Kill();
#endif
        FileSystem::Kill();
        Config::Kill();
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

    // TMP?
    std::map<uint32_t, std::shared_ptr<Model>> handled_models;
    uint32_t current_model_handle;
    std::map<uint32_t, std::shared_ptr<Light>> handled_lights;
    uint32_t current_light_handle;
};

Engine::SceneControl::SceneControl(std::shared_ptr<EngineImpl> impl):
eimpl_(impl)
{

}

Engine::PipelineControl::PipelineControl(std::shared_ptr<EngineImpl> impl):
eimpl_(impl)
{

}

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
        DLOGE("[Engine] Stream exception while initializing CONFIG singleton.", "core", Severity::CRIT);
        DLOGI(e.what(), "core", Severity::CRIT);
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
        DLOGE("[Engine] Stream exception while initializing intern string solver HRESOLVE.", "core", Severity::CRIT);
        DLOGI(e.what(), "core", Severity::CRIT);
    }
#endif

    eimpl_ = std::shared_ptr<EngineImpl>(new EngineImpl);
    scene_control = new SceneControl(eimpl_);
    pipeline_control = new PipelineControl(eimpl_);
}

Engine::~Engine()
{

}

bool Engine::UseResourceArchive(const char* filename, hash_t key)
{
    fs::path filepath;
    if(!CONFIG.get("root.folders.res"_h, filepath))
    {
        DLOGE("[Engine] Config node folders.res is not set.", "core", Severity::CRIT);
        return false;
    }

    filepath /= filename;
    if(!fs::exists(filepath))
    {
        DLOGE("[Engine] Cannot open esource archive:", "core", Severity::CRIT);
        DLOGI("<p>" + filepath.string() + "</p>", "core", Severity::CRIT);
        return false;
    }

    DLOGN("[Engine] Opening resource archive: ", "core", Severity::LOW);
    DLOGI("<p>" + filepath.string() + "</p>", "core", Severity::LOW);
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
        DLOGE("[Engine] Stream exception while initializing system parameters.", "core", Severity::CRIT);
        DLOGI(e.what(), "core", Severity::CRIT);
        DLOGI("Initializer systems may use default state.", "core", Severity::CRIT);
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
        DLOGF("[Engine] Stream exception while serializing system parameters.", "core", Severity::CRIT);
        DLOGI(e.what(), "core", Severity::CRIT);
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

void Engine::SceneControl::LoadStart()
{
    // Load level
    eimpl_->scene_loader->load_level(GLB.START_LEVEL);
    eimpl_->scene_loader->load_global(*eimpl_->daylight);
    eimpl_->chunk_manager->load_start();

#ifdef __DEBUG__
    show_driver_error("post LoadStart() glGetError(): ");
#endif
}

void Engine::SceneControl::LoadLevel()
{
    // Load level
    eimpl_->scene_loader->load_level(GLB.START_LEVEL);
    eimpl_->scene_loader->load_global(*eimpl_->daylight);
}

uint32_t Engine::SceneControl::LoadChunk(uint32_t xx, uint32_t zz, bool send_geometry)
{
    return eimpl_->scene_loader->load_chunk(math::i32vec2(xx,zz), send_geometry);
}

bool Engine::SceneControl::VisitRefModel(hash_t href, std::function<void(Model& model)> visit)
{
    if(auto pmdl = eimpl_->scene->get_model_by_ref(href).lock())
    {
        visit(*pmdl);
        return true;
    }
    return false;
}

void Engine::SceneControl::SendChunk(uint32_t xx, uint32_t zz)
{
    uint32_t chunk_index = std::hash<math::i32vec2>{}(math::i32vec2(xx,zz));
    eimpl_->scene->populate_static_octree(chunk_index);
    eimpl_->scene->load_geometry(chunk_index);
}



uint32_t Engine::SceneControl::LoadModel(hash_t name, uint32_t chunk_index)
{
    auto pmdl = eimpl_->scene_loader->load_model_instance(name, chunk_index);
    eimpl_->handled_models.insert(std::pair(eimpl_->current_model_handle, pmdl));
    return eimpl_->current_model_handle++;
}

void Engine::SceneControl::SetModelPosition(uint32_t model_index, const math::vec3& position)
{
    std::shared_ptr<Model> pmdl = eimpl_->handled_models.at(model_index);
    pmdl->set_position(position);
    pmdl->update_bounding_boxes();
}

void Engine::SceneControl::SetModelScale(uint32_t model_index, float scale)
{
    auto it = eimpl_->handled_models.find(model_index);
    if(it!=eimpl_->handled_models.end())
    {
        it->second->set_scale(scale);
        it->second->update_bounding_boxes();
    }
}

void Engine::SceneControl::SetModelOrientation(uint32_t model_index, const math::vec3& orientation)
{
    std::shared_ptr<Model> pmdl = eimpl_->handled_models.at(model_index);
    pmdl->set_orientation(math::quat(orientation.z(),
                          orientation.y(),
                          orientation.x()));
    pmdl->update_bounding_boxes();
}

uint32_t Engine::SceneControl::LoadPointLight(uint32_t chunk_index)
{
    auto plight = eimpl_->scene_loader->load_point_light(chunk_index);
    eimpl_->handled_lights.insert(std::pair(eimpl_->current_light_handle, plight));
    return eimpl_->current_light_handle++;
}

void Engine::SceneControl::SetLightPosition(uint32_t light_index, const math::vec3& value)
{
    auto it = eimpl_->handled_lights.find(light_index);
    if(it!=eimpl_->handled_lights.end())
        it->second->set_position(value);
}

void Engine::SceneControl::SetLightColor(uint32_t light_index, const math::vec3& value)
{
    auto it = eimpl_->handled_lights.find(light_index);
    if(it!=eimpl_->handled_lights.end())
        it->second->set_color(value);
}

void Engine::SceneControl::SetLightRadius(uint32_t light_index, float value)
{
    auto it = eimpl_->handled_lights.find(light_index);
    if(it!=eimpl_->handled_lights.end())
        it->second->set_radius(value);
}

void Engine::SceneControl::SetLightBrightness(uint32_t light_index, float value)
{
    auto it = eimpl_->handled_lights.find(light_index);
    if(it!=eimpl_->handled_lights.end())
        it->second->set_brightness(value);
}



void Engine::PipelineControl::SetDefaultFrameBuffer(unsigned int index)
{
    FrameBuffer::set_default_framebuffer(index);
    GFX::set_default_framebuffer(index);
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


}
