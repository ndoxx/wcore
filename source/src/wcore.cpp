#include "wcore.h"

#include "gfx_driver.h" // Won't compile if removed: ensures GLFW header included before GL
#include "error.h"
#include "config.h"
#include "engine_core.h"
#include "scene.h"
#include "chunk_manager.h"
#include "scene_loader.h"
#include "input_handler.h"
#include "pipeline.h"
#include "daylight.h"
#include "globals.h"
#include "logger.h"

namespace wcore
{

static void warn_global_not_found(hashstr_t name)
{
    DLOGW("Global name not found:", "core", Severity::WARN);
    DLOGI(std::to_string(name), "core", Severity::WARN);
    DLOGW("Skipping.", "core", Severity::WARN);
}

void GlobalsSet(hashstr_t name, const void* data)
{
    switch(name)
    {
        default:
            warn_global_not_found(name);
            break;
        case HS_("SCR_W"):
            GLB.SCR_W = *reinterpret_cast<const uint32_t*>(data);
            break;
        case HS_("SCR_H"):
            GLB.SCR_H = *reinterpret_cast<const uint32_t*>(data);
            break;
        case HS_("SCR_FULL"):
            GLB.SCR_FULL = *reinterpret_cast<const bool*>(data);
            break;
        case HS_("START_LEVEL"):
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
    chunk_manager(nullptr)
    {

    }

    ~EngineImpl()
    {
        delete chunk_manager;
        delete daylight;
        delete pipeline;
        delete scene_loader;
        delete game_loop;

        Scene::Kill();
        Config::Kill();
    }

    void init()
    {
        game_loop = new GameLoop();
        scene_loader = new SceneLoader();
        pipeline = new RenderPipeline();
        daylight = new DaylightSystem(*pipeline);
        chunk_manager = new ChunkManager(*scene_loader);
    }

    GameLoop*       game_loop;
    SceneLoader*    scene_loader;
    RenderPipeline* pipeline;
    DaylightSystem* daylight;
    ChunkManager*   chunk_manager;
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

    // First, try to initialize default values using config
    wcore::CONFIG.get(wcore::HS_("root.display.width"),  wcore::GLB.SCR_W);
    wcore::CONFIG.get(wcore::HS_("root.display.height"), wcore::GLB.SCR_H);
    wcore::CONFIG.get(wcore::HS_("root.display.full"),   wcore::GLB.SCR_FULL);

    // Parse command line arguments
    if(parse_arguments)
        parse_arguments(argc, argv);

    eimpl_->init();

    // Initialize game_loop
    auto&& input_handler = eimpl_->game_loop->get_input_handler();
    eimpl_->game_loop->set_render_func([&]() { eimpl_->pipeline->render(); });
    eimpl_->game_loop->set_render_gui_func([&]() { eimpl_->pipeline->render_gui(); });

    // User input (debug)
    SCENE.subscribe(H_("input.mouse.locked"), input_handler, &Scene::onMouseEvent);
    SCENE.subscribe(H_("input.keyboard"), input_handler, &Scene::onKeyboardEvent);
    eimpl_->pipeline->subscribe(H_("input.mouse.unlocked"), input_handler, &RenderPipeline::onMouseEvent);
    eimpl_->pipeline->subscribe(H_("input.keyboard"), input_handler, &RenderPipeline::onKeyboardEvent);
    eimpl_->daylight->subscribe(H_("input.keyboard"), input_handler, &DaylightSystem::onKeyboardEvent);
    eimpl_->scene_loader->subscribe(H_("input.keyboard"), input_handler, &SceneLoader::onKeyboardEvent);
    eimpl_->chunk_manager->subscribe(H_("input.keyboard"), input_handler, &ChunkManager::onKeyboardEvent);

    //dbg::LOG.track(H_("input.mouse.locked"), input_handler);
    //dbg::LOG.track(H_("input.mouse.unlocked"), input_handler);
    //dbg::LOG.track(H_("input.mouse.focus"), input_handler);

    // Editor widgets
#ifndef __DISABLE_EDITOR__
    eimpl_->game_loop->register_editor_widget([&](){ dbg::LOG.generate_widget(); });
    eimpl_->game_loop->register_editor_widget([&](){ eimpl_->pipeline->generate_widget(); });
    eimpl_->game_loop->register_editor_widget([&](){ eimpl_->daylight->generate_widget(); });
    eimpl_->game_loop->register_editor_widget([&](){ SCENE.generate_widget(); });
#endif

    eimpl_->game_loop->register_updatable_system(*eimpl_->daylight);
    eimpl_->game_loop->register_updatable_system(SCENE);
    eimpl_->game_loop->register_updatable_system(*eimpl_->chunk_manager);

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
