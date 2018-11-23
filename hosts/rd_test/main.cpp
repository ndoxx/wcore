#include <list>

#include "arguments.h"
#include "config.h"
#include "gfx_driver.h"
#include "camera.h"
#include "gl_context.h"
#include "scene.h"
#include "chunk_manager.h"
#include "scene_loader.h"
#include "input_handler.h"
#include "game_clock.h"
//#include "message_tracker.h"
#include "pipeline.h"
#include "daylight.h"

using namespace wcore;

int main(int argc, char const *argv[])
{
    // Parse config file
    CONFIG.load_file_xml("../res/xml/config.xml");
    // Parse command line arguments
    parse_program_arguments(argc, argv);

    // Initialize logger channels
#ifdef __DEBUG__
    uint32_t texture_verbosity  = 0u,
             material_verbosity = 0u,
             model_verbosity    = 0u,
             shader_verbosity   = 0u,
             text_verbosity     = 0u,
             input_verbosity    = 0u,
             buffer_verbosity   = 0u,
             chunk_verbosity    = 0u;

    CONFIG.get(H_("root.debug.channel_verbosity.texture"),  texture_verbosity);
    CONFIG.get(H_("root.debug.channel_verbosity.material"), material_verbosity);
    CONFIG.get(H_("root.debug.channel_verbosity.model"),    model_verbosity);
    CONFIG.get(H_("root.debug.channel_verbosity.shader"),   shader_verbosity);
    CONFIG.get(H_("root.debug.channel_verbosity.text"),     text_verbosity);
    CONFIG.get(H_("root.debug.channel_verbosity.input"),    input_verbosity);
    CONFIG.get(H_("root.debug.channel_verbosity.buffer"),   buffer_verbosity);
    CONFIG.get(H_("root.debug.channel_verbosity.chunk"),    chunk_verbosity);

    dbg::LOG.register_channel("texture",  texture_verbosity);
    dbg::LOG.register_channel("material", material_verbosity);
    dbg::LOG.register_channel("model",    model_verbosity);
    dbg::LOG.register_channel("shader",   shader_verbosity);
    dbg::LOG.register_channel("text",     text_verbosity);
    dbg::LOG.register_channel("input",    input_verbosity);
    dbg::LOG.register_channel("buffer",   buffer_verbosity);
    dbg::LOG.register_channel("chunk",    chunk_verbosity);
#endif

    // Initialize context
    GLContext context;

    // Initialize scene
    SCENE;
    SceneLoader scene_loader;

    // MAIN SYSTEMS
    // Graphics pipeline
    RenderPipeline pipeline;

    // Updaters
    DaylightSystem daylight(pipeline);
    ChunkManager chunk_manager(scene_loader);

    // Editor widgets
#ifndef __DISABLE_EDITOR__
    context.add_editor_widget_generator([&](){ dbg::LOG.generate_widget(); });
    context.add_editor_widget_generator([&](){ pipeline.generate_widget(); });
    context.add_editor_widget_generator([&](){ daylight.generate_widget(); });
#endif

    std::list<Updatable*> updatables;
    updatables.push_back(static_cast<Updatable*>(&daylight));
    updatables.push_back(static_cast<Updatable*>(&SCENE));
    updatables.push_back(static_cast<Updatable*>(&chunk_manager));

    // User input
    InputHandler input_handler("../res/xml/w_keybindings.xml");
    GameClock clock;
    // MessageTracker tracker;
    //tracker.track(H_("k_tg_daysys"), input_handler);

    // LOADING
    context._setup([&](GLFWwindow* window)
    {
        // scene_loader.load_file_xml("../res/xml/crystal_scene.xml");
        scene_loader.load_file_xml("../res/xml/tree_scene.xml");
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
            context.toggle_editor_GUI_rendering();
            context.toggle_cursor();
        });
#endif

#ifdef __DEBUG__
        // Error check
        auto error = GFX::get_error();
        std::string msg = std::string("post _setup() glGetError: ") + std::to_string(error);
        if(error)
            DLOGB(msg);
        else
            DLOGG(msg);
#endif
    });

    context._update([&](GLFWwindow* window, float dt)
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
        context.imgui_new_frame();
#endif

        // GAME UPDATES
        if(clock.is_game_paused()) return;
        clock.update(dt);

        for(auto&& system: updatables)
            system->update(clock);

        // To allow frame by frame update
        clock.release_flags();
    });

    context._render([&]()
    {
        if(!clock.is_game_paused())
            pipeline.render();
    });

    auto mainLoopRet = context.main_loop();
    pipeline.dbg_show_statistics();

    // Cleanup
    Scene::Kill();
    Config::Kill();

    return mainLoopRet;
}
