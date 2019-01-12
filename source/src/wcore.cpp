#include "wcore.h"

#include "gfx_driver.h" // Won't compile if removed: ensures GLFW header included before GL
#include "error.h"
#include "config.h"
#include "intern_string.h"
#include "engine_core.h"
#include "scene.h"
#include "chunk_manager.h"
#include "scene_loader.h"
#include "input_handler.h"
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
    scene_loader(nullptr),
    pipeline(nullptr),
    daylight(nullptr),
    ray_caster(nullptr),
    chunk_manager(nullptr)
#ifndef __DISABLE_EDITOR__
    ,editor(nullptr)
#endif
    {

    }

    ~EngineImpl()
    {
        delete chunk_manager;
        delete ray_caster;
        delete daylight;
        delete pipeline;
        delete scene_loader;
        delete scene;
        delete game_loop;

#ifndef __DISABLE_EDITOR__
        delete editor;
#endif
        Config::Kill();
    }

    void init()
    {
        game_loop     = new GameLoop();
        scene         = new Scene();
        scene_loader  = new SceneLoader();
        pipeline      = new RenderPipeline();
        daylight      = new DaylightSystem();
        ray_caster    = new RayCaster();
        chunk_manager = new ChunkManager();
#ifndef __DISABLE_EDITOR__
        editor        = new Editor();
#endif

    }

    GameLoop*       game_loop;
    Scene*          scene;
    SceneLoader*    scene_loader;
    RenderPipeline* pipeline;
    DaylightSystem* daylight;
    RayCaster*      ray_caster;
    ChunkManager*   chunk_manager;
#ifndef __DISABLE_EDITOR__
    Editor*         editor;
#endif
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
    eimpl_->game_loop->register_game_system(H_("Editor"),       static_cast<GameSystem*>(eimpl_->editor));
#endif
    eimpl_->game_loop->register_game_system(H_("Scene"),        static_cast<GameSystem*>(eimpl_->scene));
    eimpl_->game_loop->register_game_system(H_("Pipeline"),     static_cast<GameSystem*>(eimpl_->pipeline));
    eimpl_->game_loop->register_game_system(H_("Daylight"),     static_cast<GameSystem*>(eimpl_->daylight));
    eimpl_->game_loop->register_game_system(H_("RayCaster"),    static_cast<GameSystem*>(eimpl_->ray_caster));
    eimpl_->game_loop->register_game_system(H_("SceneLoader"),  static_cast<GameSystem*>(eimpl_->scene_loader));
    eimpl_->game_loop->register_game_system(H_("ChunkManager"), static_cast<GameSystem*>(eimpl_->chunk_manager));

    //auto&& input_handler = eimpl_->game_loop->get_input_handler();
    //dbg::LOG.track(H_("input.mouse.locked"), input_handler);
    //dbg::LOG.track(H_("input.mouse.unlocked"), input_handler);
    //dbg::LOG.track(H_("input.mouse.focus"), input_handler);

#ifdef __DEBUG__
    show_driver_error("post Init() glGetError(): ");
#endif
}

void Engine::LoadStart()
{
    // Load level
    eimpl_->scene_loader->load_level(GLB.START_LEVEL);
    eimpl_->scene_loader->load_global(*eimpl_->daylight);
    eimpl_->chunk_manager->init();

#ifdef __DEBUG__
    show_driver_error("post LoadStart() glGetError(): ");
#endif
}

int Engine::Run()
{
    int ret = eimpl_->game_loop->run();
    eimpl_->pipeline->dbg_show_statistics();
    return ret;
}


}
