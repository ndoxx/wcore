#include <map>
#include <fstream>

#include "wcore.h"

#include "gfx_driver.h" // Won't compile if removed: ensures GLFW header included before GL
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
    game_loop(nullptr),
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
        FileSystem::Instance();
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
        delete game_loop;

        FileSystem::Kill();
        Config::Kill();
    }

    void init()
    {
        game_loop           = new GameLoop();

#ifndef __DISABLE_EDITOR__
        ed_tweaks           = new EditorTweaksInitializer();
#endif

        scene               = new Scene();
        entity_system       = new EntitySystem();
        camera_controller   = new CameraController();
        game_object_factory = new GameObjectFactory();
        scene_loader        = new SceneLoader();
        pipeline            = new RenderPipeline();
        daylight            = new DaylightSystem();
        ray_caster          = new RayCaster();
        chunk_manager       = new ChunkManager();
        sound_system        = new SoundSystem();
#ifndef __DISABLE_EDITOR__
        editor              = new Editor();
#endif

    }

    GameLoop* game_loop;

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

Engine::Engine()
{
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

    eimpl_ = std::unique_ptr<EngineImpl>(new EngineImpl);
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
                  void(*parse_arguments)(int, char const **))
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
    eimpl_->init();

    // * Initialize game_loop
    DLOG("<s>--- WCore: Bootstrapping initializer systems ---</s>", "core", Severity::LOW);
    // Render callbacks
    eimpl_->game_loop->set_render_func([&]()     { eimpl_->pipeline->render(); });
    eimpl_->game_loop->set_render_gui_func([&]() { eimpl_->pipeline->render_gui(); });
    // Initializer systems
#ifndef __DISABLE_EDITOR__
    eimpl_->game_loop->register_initializer_system("EditorTweaks"_h, static_cast<InitializerSystem*>(eimpl_->ed_tweaks));
#endif

    // Initialization step
    try
    {
        eimpl_->game_loop->init_system_parameters();
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
    eimpl_->game_loop->register_game_system("Editor"_h,            static_cast<GameSystem*>(eimpl_->editor));
#endif
    eimpl_->game_loop->register_game_system("CameraController"_h,  static_cast<GameSystem*>(eimpl_->camera_controller));
    eimpl_->game_loop->register_game_system("EntitySystem"_h,      static_cast<GameSystem*>(eimpl_->entity_system));
    eimpl_->game_loop->register_game_system("Scene"_h,             static_cast<GameSystem*>(eimpl_->scene));
    eimpl_->game_loop->register_game_system("Pipeline"_h,          static_cast<GameSystem*>(eimpl_->pipeline));
    eimpl_->game_loop->register_game_system("Daylight"_h,          static_cast<GameSystem*>(eimpl_->daylight));
    eimpl_->game_loop->register_game_system("RayCaster"_h,         static_cast<GameSystem*>(eimpl_->ray_caster));
    eimpl_->game_loop->register_game_system("GameObjectFactory"_h, static_cast<GameSystem*>(eimpl_->game_object_factory));
    eimpl_->game_loop->register_game_system("SceneLoader"_h,       static_cast<GameSystem*>(eimpl_->scene_loader));
    eimpl_->game_loop->register_game_system("ChunkManager"_h,      static_cast<GameSystem*>(eimpl_->chunk_manager));
    eimpl_->game_loop->register_game_system("SoundSystem"_h,       static_cast<GameSystem*>(eimpl_->sound_system));

    DLOG("<s>--- WCore: Initializing game systems ---</s>", "core", Severity::LOW);
    eimpl_->game_loop->init_game_systems();

    //auto&& input_handler = eimpl_->game_loop->get_input_handler();
    //dbg::LOG.track("input.mouse.locked"_h, input_handler);
    //dbg::LOG.track("input.mouse.unlocked"_h, input_handler);
    //dbg::LOG.track("input.mouse.focus"_h, input_handler);

    DLOG("<s>--- WCore: Done ---</s>", "core", Severity::LOW);
#ifdef __DEBUG__
    show_driver_error("post Init() glGetError(): ");
#endif
}

uint32_t Engine::LoadChunk(uint32_t xx, uint32_t zz, bool send_geometry)
{
    return eimpl_->scene_loader->load_chunk(math::i32vec2(xx,zz), send_geometry);
}

void Engine::SendChunk(uint32_t xx, uint32_t zz)
{
    uint32_t chunk_index = std::hash<math::i32vec2>{}(math::i32vec2(xx,zz));
    eimpl_->scene->populate_static_octree(chunk_index);
    eimpl_->scene->load_geometry(chunk_index);
}

void Engine::LoadLevel()
{
    // Load level
    eimpl_->scene_loader->load_level(GLB.START_LEVEL);
    eimpl_->scene_loader->load_global(*eimpl_->daylight);
}

void Engine::LoadStart()
{
    // Load level
    eimpl_->scene_loader->load_level(GLB.START_LEVEL);
    eimpl_->scene_loader->load_global(*eimpl_->daylight);
    eimpl_->chunk_manager->load_start();

#ifdef __DEBUG__
    show_driver_error("post LoadStart() glGetError(): ");
#endif
}

uint32_t Engine::LoadModel(hash_t name, uint32_t chunk_index)
{
    auto pmdl = eimpl_->scene_loader->load_model_instance(name, chunk_index);
    eimpl_->handled_models.insert(std::pair(eimpl_->current_model_handle, pmdl));
    return eimpl_->current_model_handle++;
}

void Engine::SetModelPosition(uint32_t model_index, const math::vec3& position)
{
    std::shared_ptr<Model> pmdl = eimpl_->handled_models.at(model_index);
    pmdl->set_position(position);
    pmdl->update_bounding_boxes();
}

void Engine::SetModelScale(uint32_t model_index, float scale)
{
    auto it = eimpl_->handled_models.find(model_index);
    if(it!=eimpl_->handled_models.end())
    {
        it->second->set_scale(scale);
        it->second->update_bounding_boxes();
    }
}

void Engine::SetModelOrientation(uint32_t model_index, const math::vec3& orientation)
{
    std::shared_ptr<Model> pmdl = eimpl_->handled_models.at(model_index);
    pmdl->set_orientation(math::quat(orientation.z(),
                          orientation.y(),
                          orientation.x()));
    pmdl->update_bounding_boxes();
}

uint32_t Engine::LoadPointLight(uint32_t chunk_index)
{
    auto plight = eimpl_->scene_loader->load_point_light(chunk_index);
    eimpl_->handled_lights.insert(std::pair(eimpl_->current_light_handle, plight));
    return eimpl_->current_light_handle++;
}

void Engine::SetLightPosition(uint32_t light_index, const math::vec3& value)
{
    auto it = eimpl_->handled_lights.find(light_index);
    if(it!=eimpl_->handled_lights.end())
        it->second->set_position(value);
}

void Engine::SetLightColor(uint32_t light_index, const math::vec3& value)
{
    auto it = eimpl_->handled_lights.find(light_index);
    if(it!=eimpl_->handled_lights.end())
        it->second->set_color(value);
}

void Engine::SetLightRadius(uint32_t light_index, float value)
{
    auto it = eimpl_->handled_lights.find(light_index);
    if(it!=eimpl_->handled_lights.end())
        it->second->set_radius(value);
}

void Engine::SetLightBrightness(uint32_t light_index, float value)
{
    auto it = eimpl_->handled_lights.find(light_index);
    if(it!=eimpl_->handled_lights.end())
        it->second->set_brightness(value);
}


int Engine::Run()
{
    int ret = eimpl_->game_loop->run();
#ifdef __DEBUG__
    eimpl_->pipeline->dbg_show_statistics();
#endif
    try
    {
        eimpl_->game_loop->serialize_system_parameters();
        eimpl_->game_loop->unload_game_systems();
    }
    catch (const std::ofstream::failure& e)
    {
        DLOGF("[Engine] Stream exception while serializing system parameters.", "core", Severity::CRIT);
        DLOGI(e.what(), "core", Severity::CRIT);
    }
    return ret;
}


}
