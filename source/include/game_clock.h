#ifndef GAME_CLOCK_H
#define GAME_CLOCK_H

class InputHandler;

class GameClock
{
private:
    float frame_speed_;
    float dt_;
    bool next_frame_required_;
    bool pause_;

    static float MAX_FRAME_SPEED_;
    static float SPEED_INCREMENT_;

public:
    GameClock();
    ~GameClock() = default;

    void setup_user_inputs(InputHandler& handler);

    inline bool is_next_frame_required() const { return next_frame_required_; }
    inline bool is_game_paused() const { return pause_; }
    inline float get_frame_speed() const { return next_frame_required_?1.0f:frame_speed_; }

    inline void toggle_pause() { pause_ = ! pause_; }
    inline void set_frame_speed(float value) { frame_speed_ = value; }
    inline void require_next_frame() { if(frame_speed_ == 0.0f) next_frame_required_ = true; }

    inline void frame_speed_up()
    {
        frame_speed_ += SPEED_INCREMENT_;
        if(frame_speed_ > MAX_FRAME_SPEED_)
            frame_speed_ = MAX_FRAME_SPEED_;
    }
    inline void frame_slow_down()
    {
        frame_speed_ -= SPEED_INCREMENT_;
        if(frame_speed_ < 0.0f)
            frame_speed_ = 0.0f;
    }

    inline void update(float dt) { dt_ = dt; }
    inline void release_flags() { next_frame_required_ = false; }
    inline float get_scaled_frame_duration() const { return get_frame_speed()*dt_; }
    inline float get_frame_duration() const { return dt_; }

};

#endif // GAME_CLOCK_H
