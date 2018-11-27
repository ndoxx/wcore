#include "wcore.h"

#include "gfx_driver.h" // Won't compile if removed
#include "error.h"
#include "arguments.h"
#include "config.h"
#include "engine_core.h"
#include "scene.h"
#include "chunk_manager.h"
#include "scene_loader.h"
#include "input_handler.h"
#include "pipeline.h"
#include "daylight.h"
#include "globals.h"


namespace wcore
{

struct Engine::EngineResources
{
    EngineResources():
    game_loop(nullptr),
    scene_loader(nullptr),
    pipeline(nullptr),
    daylight(nullptr),
    chunk_manager(nullptr)
    {

    }

    ~EngineResources()
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
eres_(new EngineResources)
{

}

Engine::~Engine()
{

}


void Engine::Init(int argc, char const *argv[],
                  std::function<void(int, char const **)> argument_parser)
{
    // Parse config file
    CONFIG.init();
    // Parse command line arguments
    argument_parser(argc, argv);

    eres_->init();

    // Initialize game_loop
    auto&& input_handler = eres_->game_loop->get_input_handler();
    eres_->game_loop->set_render_func([&]() { eres_->pipeline->render(); });

    // User input
    SCENE.subscribe(H_("input.mouse"), input_handler, &Scene::onMouseEvent);
    SCENE.subscribe(H_("input.keyboard"), input_handler, &Scene::onKeyboardEvent);
    eres_->pipeline->subscribe(H_("input.keyboard"), input_handler, &RenderPipeline::onKeyboardEvent);
    eres_->daylight->subscribe(H_("input.keyboard"), input_handler, &DaylightSystem::onKeyboardEvent);
    eres_->scene_loader->subscribe(H_("input.keyboard"), input_handler, &SceneLoader::onKeyboardEvent);
    eres_->chunk_manager->subscribe(H_("input.keyboard"), input_handler, &ChunkManager::onKeyboardEvent);

    // Editor widgets
#ifndef __DISABLE_EDITOR__
    eres_->game_loop->register_editor_widget([&](){ dbg::LOG.generate_widget(); });
    eres_->game_loop->register_editor_widget([&](){ eres_->pipeline->generate_widget(); });
    eres_->game_loop->register_editor_widget([&](){ eres_->daylight->generate_widget(); });
#endif

    eres_->game_loop->register_updatable_system(*eres_->daylight);
    eres_->game_loop->register_updatable_system(SCENE);
    eres_->game_loop->register_updatable_system(*eres_->chunk_manager);

#ifdef __DEBUG__
    show_driver_error("post Init() glGetError(): ");
#endif
}

void Engine::LoadStart()
{
    // Load level
    eres_->scene_loader->load_level(GLB.START_LEVEL);
    eres_->scene_loader->load_global(*eres_->daylight);
    eres_->chunk_manager->init();

#ifdef __DEBUG__
    show_driver_error("post LoadStart() glGetError(): ");
#endif
}

int Engine::Run()
{
    int ret = eres_->game_loop->run();
    eres_->pipeline->dbg_show_statistics();
    return ret;
}


}
