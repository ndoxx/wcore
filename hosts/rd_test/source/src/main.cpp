#include <list>

#include "arguments.h"
#include "config.h"
#include "gfx_driver.h"
#include "camera.h"
#include "engine_core.h"
#include "scene.h"
#include "chunk_manager.h"
#include "scene_loader.h"
#include "input_handler.h"
#include "game_clock.h"
//#include "message_tracker.h"
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

    // Updaters
    DaylightSystem daylight(pipeline);
    ChunkManager chunk_manager(scene_loader);

    // Editor widgets
#ifndef __DISABLE_EDITOR__
    game_loop.add_editor_widget_generator([&](){ dbg::LOG.generate_widget(); });
    game_loop.add_editor_widget_generator([&](){ pipeline.generate_widget(); });
    game_loop.add_editor_widget_generator([&](){ daylight.generate_widget(); });
#endif

    std::list<Updatable*> updatables;
    updatables.push_back(static_cast<Updatable*>(&daylight));
    updatables.push_back(static_cast<Updatable*>(&SCENE));
    updatables.push_back(static_cast<Updatable*>(&chunk_manager));

    // User input
    InputHandler input_handler;
    GameClock clock;
    // MessageTracker tracker;
    //tracker.track(H_("k_tg_daysys"), input_handler);

    // Setup
    scene_loader.load_level(GLB.START_LEVEL);
    scene_loader.load_global(daylight);
    chunk_manager.init();
    // Map key bindings
    SCENE.setup_user_inputs(input_handler);
    pipeline.setup_user_inputs(input_handler);
    daylight.setup_user_inputs(input_handler);
    scene_loader.setup_user_inputs(input_handler);
    chunk_manager.setup_user_inputs(input_handler);
    clock.setup_user_inputs(input_handler);

#ifndef __DISABLE_EDITOR__
    input_handler.register_action(H_("k_tg_editor"), [&]()
    {
        input_handler.toggle_mouse_lock();
        game_loop.toggle_editor_GUI_rendering();
        game_loop.toggle_cursor();
    });
#endif

#ifdef __DEBUG__
    // Error check
    auto error = GFX::get_error();
    std::string msg = std::string("post _setup() glGetError: ") + std::to_string(error);
    if(error)
        DLOGB(msg, "core", Severity::CRIT);
    else
        DLOGG(msg, "core", Severity::LOW);
#endif

    game_loop._update([&](GLFWwindow* window, float dt)
    {
        // USER INPUTS
        input_handler.handle_mouse(window, [&](float dx, float dy)
        {
            if(input_handler.is_mouse_locked())
                SCENE.get_camera()->update_orientation(dx, dy);
        });
        input_handler.handle_keybindings(window);

        // Start the Dear ImGui frame
#ifndef __DISABLE_EDITOR__
        game_loop.imgui_new_frame();
#endif

        // GAME UPDATES
        if(clock.is_game_paused()) return;
        clock.update(dt);

        for(auto&& system: updatables)
            system->update(clock);

        // To allow frame by frame update
        clock.release_flags();
    });

    game_loop._render([&]()
    {
        if(!clock.is_game_paused())
            pipeline.render();
    });

    auto mainLoopRet = game_loop.main_loop();
    pipeline.dbg_show_statistics();

    // Cleanup
    Scene::Kill();
    Config::Kill();

    return mainLoopRet;
}
