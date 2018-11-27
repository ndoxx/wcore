#include <list>

#include "arguments.h"
#include "config.h"
#include "gfx_driver.h"
#include "engine_core.h"
#include "scene.h"
#include "chunk_manager.h"
#include "scene_loader.h"
#include "input_handler.h"
#include "pipeline.h"
#include "daylight.h"
#include "globals.h"

using namespace wcore;

int main(int argc, char const *argv[])
{
    // Parse config file
    CONFIG.init();
    // Parse command line arguments
    rd_test::parse_program_arguments(argc, argv);

    // Initialize game_loop
    GameLoop game_loop;

    // Initialize scene
    SCENE; // Not really needed
    SceneLoader scene_loader;

    // MAIN SYSTEMS
    // Graphics pipeline
    RenderPipeline pipeline;
    game_loop.set_render_func([&]() { pipeline.render(); });

    // Updaters
    DaylightSystem daylight(pipeline);
    ChunkManager chunk_manager(scene_loader);

    // Editor widgets
#ifndef __DISABLE_EDITOR__
    game_loop.register_editor_widget([&](){ dbg::LOG.generate_widget(); });
    game_loop.register_editor_widget([&](){ pipeline.generate_widget(); });
    game_loop.register_editor_widget([&](){ daylight.generate_widget(); });
#endif

    game_loop.register_updatable_system(static_cast<Updatable*>(&daylight));
    game_loop.register_updatable_system(static_cast<Updatable*>(&SCENE));
    game_loop.register_updatable_system(static_cast<Updatable*>(&chunk_manager));

    // Setup
    scene_loader.load_level(GLB.START_LEVEL);
    scene_loader.load_global(daylight);
    chunk_manager.init();

    // User input
    SCENE.subscribe(H_("input.mouse"), game_loop.get_input_handler(), &Scene::onMouseEvent);
    SCENE.subscribe(H_("input.keyboard"), game_loop.get_input_handler(), &Scene::onKeyboardEvent);
    pipeline.subscribe(H_("input.keyboard"), game_loop.get_input_handler(), &RenderPipeline::onKeyboardEvent);
    daylight.subscribe(H_("input.keyboard"), game_loop.get_input_handler(), &DaylightSystem::onKeyboardEvent);
    scene_loader.subscribe(H_("input.keyboard"), game_loop.get_input_handler(), &SceneLoader::onKeyboardEvent);
    chunk_manager.subscribe(H_("input.keyboard"), game_loop.get_input_handler(), &ChunkManager::onKeyboardEvent);

#ifdef __DEBUG__
    // Error check
    auto error = GFX::get_error();
    std::string msg = std::string("post _setup() glGetError: ") + std::to_string(error);
    if(error)
        DLOGB(msg, "core", Severity::CRIT);
    else
        DLOGG(msg, "core", Severity::LOW);
#endif

    auto mainLoopRet = game_loop.main_loop();
    pipeline.dbg_show_statistics();

    // Cleanup
    Scene::Kill();
    Config::Kill();

    return mainLoopRet;
}
