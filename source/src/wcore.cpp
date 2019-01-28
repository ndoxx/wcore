#include <map>

#include "wcore.h"

#include "gfx_driver.h" // Won't compile if removed: ensures GLFW header included before GL
#include "error.h"
#include "config.h"
#include "intern_string.h"
#include "engine_core.h"
#include "scene.h"
#include "model.h"
#include "lights.h"
#include "chunk_manager.h"
#include "camera_controller.h"
#include "scene_loader.h"
#include "game_object_factory.h"
#include "input_handler.h"
#include "sound_system.h"
#include "pipeline.h"
#include "daylight.h"
#include "ray_caster.h"
#ifndef __DISABLE_EDITOR__
#include "editor.h"
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
        case H_("SCR_W"):
            GLB.SCR_W = *reinterpret_cast<const uint32_t*>(data);
            break;
        case H_("SCR_H"):
            GLB.SCR_H = *reinterpret_cast<const uint32_t*>(data);
            break;
        case H_("SCR_FULL"):
            GLB.SCR_FULL = *reinterpret_cast<const bool*>(data);
            break;
        case H_("START_LEVEL"):
            char* value = const_cast<char*>(reinterpret_cast<const char*>(data));
            GLB.START_LEVEL = value;
            break;
    }
}

struct Engine::EngineImpl
{
    EngineImpl():
    game_loop(nullptr),
    scene(nullptr),
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

    }

    ~EngineImpl()
    {
#ifndef __DISABLE_EDITOR__
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
        delete scene;
        delete game_loop;

        Config::Kill();
    }

    void init()
    {
        game_loop           = new GameLoop();
        scene               = new Scene();
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

    GameLoop*          game_loop;
    Scene*             scene;
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

Engine::Engine():
eimpl_(new EngineImpl)
{

}

Engine::~Engine()
{

}


void Engine::Init(int argc, char const *argv[],
                  void(*parse_arguments)(int, char const **))
{
    // Parse config file
    CONFIG.init();

    // Parse intern string hash table file
    #ifdef __DEBUG__
        HRESOLVE.init();
    #endif

    // First, try to initialize default values using config
    wcore::CONFIG.get(wcore::H_("root.display.width"),  wcore::GLB.SCR_W);
    wcore::CONFIG.get(wcore::H_("root.display.height"), wcore::GLB.SCR_H);
    wcore::CONFIG.get(wcore::H_("root.display.full"),   wcore::GLB.SCR_FULL);

    // Parse command line arguments
    if(parse_arguments)
        parse_arguments(argc, argv);

    // Create game systems
    eimpl_->init();

    // Initialize game_loop
    eimpl_->game_loop->set_render_func([&]()     { eimpl_->pipeline->render(); });
    eimpl_->game_loop->set_render_gui_func([&]() { eimpl_->pipeline->render_gui(); });

    // Register game systems (init events, register editor widgets, add to update list)
#ifndef __DISABLE_EDITOR__
    eimpl_->game_loop->register_game_system(H_("Editor"),            static_cast<GameSystem*>(eimpl_->editor));
#endif
    eimpl_->game_loop->register_game_system(H_("CameraController"),  static_cast<GameSystem*>(eimpl_->camera_controller));
    eimpl_->game_loop->register_game_system(H_("Scene"),             static_cast<GameSystem*>(eimpl_->scene));
    eimpl_->game_loop->register_game_system(H_("Pipeline"),          static_cast<GameSystem*>(eimpl_->pipeline));
    eimpl_->game_loop->register_game_system(H_("Daylight"),          static_cast<GameSystem*>(eimpl_->daylight));
    eimpl_->game_loop->register_game_system(H_("RayCaster"),         static_cast<GameSystem*>(eimpl_->ray_caster));
    eimpl_->game_loop->register_game_system(H_("GameObjectFactory"), static_cast<GameSystem*>(eimpl_->game_object_factory));
    eimpl_->game_loop->register_game_system(H_("SceneLoader"),       static_cast<GameSystem*>(eimpl_->scene_loader));
    eimpl_->game_loop->register_game_system(H_("ChunkManager"),      static_cast<GameSystem*>(eimpl_->chunk_manager));
    eimpl_->game_loop->register_game_system(H_("SoundSystem"),       static_cast<GameSystem*>(eimpl_->sound_system));

    // TMP
    eimpl_->camera_controller->register_camera(eimpl_->scene->get_camera());

    //auto&& input_handler = eimpl_->game_loop->get_input_handler();
    //dbg::LOG.track(H_("input.mouse.locked"), input_handler);
    //dbg::LOG.track(H_("input.mouse.unlocked"), input_handler);
    //dbg::LOG.track(H_("input.mouse.focus"), input_handler);

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
    eimpl_->pipeline->dbg_show_statistics();
    return ret;
}


}
