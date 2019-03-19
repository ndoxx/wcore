#include "camera_controller.h"
#include "message.h"
#include "camera.h"
#include "scene.h"
#include "model.h"
#include "bounding_boxes.h"
#include "input_handler.h"
#include "game_clock.h"

namespace wcore
{

CameraController::CameraController():
recording_(false),
frame_count_(0)
{
    // Add freefly camera as first mode
    //     tracking shot as second mode
    CameraState* dbg_freefly  = new CameraStateFreefly();
    CameraState* dbg_tracking = new CameraStateTrackingShot();
    camera_states_.push_back(dbg_freefly);
    camera_states_.push_back(dbg_tracking);
    current_state_ = CameraStateIndex::FREEFLY; // 0
}

CameraController::~CameraController()
{
    for(CameraState* cs: camera_states_)
        delete cs;
}

void CameraController::init_events(InputHandler& handler)
{
    subscribe("input.mouse.locked"_h, handler, &CameraController::onMouseEvent);
    subscribe("input.keyboard"_h, handler, &CameraController::onKeyboardEvent);
}

void CameraController::init_self()
{
    // Register scene default camera
    register_camera(locate<Scene>("Scene"_h)->get_camera_shared());
}

void CameraController::update(const GameClock& clock)
{
    float dt = clock.get_frame_duration();

    // Motion recording
    if(recording_ && frame_count_++%10 && current_state_ == CameraStateIndex::FREEFLY)
    {
        CameraStateTrackingShot* ts_state
            = static_cast<CameraStateTrackingShot*>(camera_states_[CameraStateIndex::TRACKING]);

        const math::vec3& pos = camera_->get_position();
        float pitch = camera_->get_pitch();
        float yaw   = camera_->get_yaw();
        math::quat q(0.f, pitch, yaw);

        float dp = (pos-ts_state->last_position()).norm();
        float dq = 1.f - std::abs(q.dot_vector(ts_state->last_orientation()));

        if(dp>1.f || dq>0.05f)
        {
            std::cout << dp << " " << dq << std::endl;
            std::cout << frame_count_ << " new keyframe" << std::endl;
            ts_state->add_keyframe(pos, q);
        }
    }

    // Update camera
    current_state()->control(*camera_, dt);
    camera_->update(dt);
}

void CameraController::register_camera(std::shared_ptr<Camera> camera)
{
    camera_ = camera;
    camera_->update(1.f/60.f);
}

bool CameraController::onKeyboardEvent(const WData& data)
{
    // * First, handle events that target this system
    // Switch state based on kbd input for now
    const KbdData& kbd = static_cast<const KbdData&>(data);
    if(kbd.key_binding == "k_cam_new_keyframe"_h)
    {
        CameraStateTrackingShot* ts_state
            = static_cast<CameraStateTrackingShot*>(camera_states_[CameraStateIndex::TRACKING]);
        ts_state->add_keyframe(camera_->get_position(),
                               math::quat(0.f,camera_->get_pitch(),camera_->get_yaw()));
        return true;
    }
    else if(kbd.key_binding == "k_cam_tg_record"_h)
    {
        CameraStateTrackingShot* ts_state
            = static_cast<CameraStateTrackingShot*>(camera_states_[CameraStateIndex::TRACKING]);
        ts_state->add_keyframe(camera_->get_position(),
                               math::quat(0.f,camera_->get_pitch(),camera_->get_yaw()));
        recording_ = !recording_;
        if(recording_)
        {
            frame_count_ = 0;
        }
        return true;
    }
    else if(kbd.key_binding == "k_cam_gen_track"_h)
    {
        recording_ = false;
        CameraStateTrackingShot* ts_state
            = static_cast<CameraStateTrackingShot*>(camera_states_[CameraStateIndex::TRACKING]);
        ts_state->generate_interpolator();
        return true;
    }
    else if(kbd.key_binding == "k_cam_next_state"_h)
    {
        // TMP just advance state index
        current_state_ = (current_state_+1)%camera_states_.size();
        current_state()->on_load();
        return true;
    }

    // * Propagate events down to current camera state object
    return current_state()->onKeyboardEvent(data, *camera_);
}

bool CameraController::onMouseEvent(const WData& data)
{
    return current_state()->onMouseEvent(data, *camera_);
}

void CameraStateFreefly::control(Camera& camera, float dt)
{
    camera.freefly_view();
}

bool CameraStateFreefly::onMouseEvent(const WData& data, Camera& camera)
{
    const MouseData& md = static_cast<const MouseData&>(data);
    camera.update_orientation(-md.dx, -md.dy);

    return true; // Do NOT consume event
}

bool CameraStateFreefly::onKeyboardEvent(const WData& data, Camera& camera)
{
    const KbdData& kbd = static_cast<const KbdData&>(data);

    switch(kbd.key_binding)
    {
        case "k_run"_h:
            camera.set_speed_fast();
            break;
        case "k_walk"_h:
            camera.set_speed_slow();
            break;
        case "k_forward"_h:
            camera.move_forward();
            break;
        case "k_backward"_h:
            camera.move_backward();
            break;
        case "k_strafe_left"_h:
            camera.strafe_left();
            break;
        case "k_strafe_right"_h:
            camera.strafe_right();
            break;
        case "k_ascend"_h:
            camera.ascend();
            break;
        case "k_descend"_h:
            camera.descend();
            break;
    }

    return true; // Do NOT consume event
}

CameraStateTrackingShot::CameraStateTrackingShot():
position_interpolator_(nullptr),
orientation_interpolator_(nullptr),
t_(0.f),
max_t_(0.f),
speed_(5.f)
{

}

CameraStateTrackingShot::~CameraStateTrackingShot()
{
    if(position_interpolator_)
        delete position_interpolator_;
    if(orientation_interpolator_)
        delete orientation_interpolator_;
}

bool CameraStateTrackingShot::onMouseEvent(const WData& data, Camera& camera)
{
    //const MouseData& md = static_cast<const MouseData&>(data);

    return true; // Do NOT consume event
}

bool CameraStateTrackingShot::onKeyboardEvent(const WData& data, Camera& camera)
{
    const KbdData& kbd = static_cast<const KbdData&>(data);

    switch(kbd.key_binding)
    {
        case "k_tc_faster"_h:
            speed_ = std::min(10.f, speed_+0.5f);
            break;
        case "k_tc_slower"_h:
            speed_ = std::max(0.5f, speed_-0.5f);
            break;
    }

    return true; // Do NOT consume event
}

void CameraStateTrackingShot::control(Camera& camera, float dt)
{
    if(!position_interpolator_ || !orientation_interpolator_) return;

    math::vec3 newpos(position_interpolator_->interpolate(t_));
    math::quat newori(orientation_interpolator_->interpolate(t_));
    math::vec3 euler = newori.get_euler_angles();

    camera.set_position(newpos);
    camera.set_orientation(euler.x(),euler.y());

    t_ += speed_*dt;
    if(t_ > max_t_)
        t_ = 0.f;
}

void CameraStateTrackingShot::on_load()
{
    t_ = 0.f; // Reset current parameter value
    speed_ = 5.f;
}


void CameraStateTrackingShot::add_keyframe(const math::vec3& position,
                                           const math::quat& orientation)
{
    float parameter = 0.f;
    if(key_frame_positions_.size()>0)
        parameter = key_frame_parameters_.back() + (position-key_frame_positions_.back()).norm();

    key_frame_positions_.push_back(position);
    key_frame_orientations_.push_back(orientation);
    key_frame_parameters_.push_back(parameter);
}

void CameraStateTrackingShot::generate_interpolator()
{
    if(key_frame_parameters_.size() == 0) return;

    // Loop back to first keyframe?
    add_keyframe(key_frame_positions_[0], key_frame_orientations_[0]);

    if(position_interpolator_)
        delete position_interpolator_;
    if(orientation_interpolator_)
        delete orientation_interpolator_;

    position_interpolator_ = new math::CSplineCardinalV3(key_frame_parameters_,
                                                         key_frame_positions_);
    orientation_interpolator_ = new SlerpInterpolator(key_frame_parameters_,
                                                      key_frame_orientations_);

    max_t_ = key_frame_parameters_.back();

    key_frame_positions_.clear();
    key_frame_orientations_.clear();
    key_frame_parameters_.clear();
}


CameraStateCircleAround::CameraStateCircleAround():
radius_(1.f),
//speed_(1.f),
t_(0.f)
{

}

void CameraStateCircleAround::follow(std::weak_ptr<Model> target, Camera& camera)
{
    wtarget_ = target;
    if(auto ptarget = wtarget_.lock())
    {
        lookat_pos_ = traits::center<OBB>::get(ptarget->get_OBB());
        auto& cam_pos = camera.get_position();
        radius_ = (lookat_pos_-cam_pos).norm();
    }
}


bool CameraStateCircleAround::onMouseEvent(const WData& data, Camera& camera)
{
    return true; // Do NOT consume event
}

bool CameraStateCircleAround::onKeyboardEvent(const WData& data, Camera& camera)
{
    return true; // Do NOT consume event
}

void CameraStateCircleAround::control(Camera& camera, float dt)
{

}

void CameraStateCircleAround::on_load()
{
    t_ = 0.f;
}



} // namespace wcore
