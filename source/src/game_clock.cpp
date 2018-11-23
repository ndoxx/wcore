#include "game_clock.h"
#include "input_handler.h"

namespace wcore
{

float GameClock::MAX_FRAME_SPEED_ = 5.0f;
float GameClock::SPEED_INCREMENT_ = 0.1f;

GameClock::GameClock():
frame_speed_(1.0f),
dt_(0.0f),
next_frame_required_(false),
pause_(false)
{

}

void GameClock::setup_user_inputs(InputHandler& handler)
{
    handler.register_action(H_("k_tg_pause"), [&]()
    {
        toggle_pause();
    });
    handler.register_action(H_("k_frame_speed_up"), [&]()
    {
        frame_speed_up();
    });
    handler.register_action(H_("k_frame_slow_down"), [&]()
    {
        frame_slow_down();
    });
    handler.register_action(H_("k_next_frame"), [&]()
    {
        require_next_frame();
    });
}

}
