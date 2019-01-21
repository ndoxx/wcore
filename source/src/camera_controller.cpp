#include "camera_controller.h"
#include "message.h"
#include "camera.h"
#include "scene.h"
#include "input_handler.h"
#include "game_clock.h"

namespace wcore
{

CameraController::CameraController()
{
    // Add freefly camera as first mode
    //     tracking shot as second mode
    CameraState* dbg_freefly  = new CameraStateFreefly();
    CameraState* dbg_tracking = new CameraStateTrackingShot();
    camera_states_.push_back(dbg_freefly);
    camera_states_.push_back(dbg_tracking);
    current_state_ = 0;
}

CameraController::~CameraController()
{
    for(CameraState* cs: camera_states_)
        delete cs;
}

void CameraController::init_events(InputHandler& handler)
{
    subscribe(H_("input.mouse.locked"), handler, &CameraController::onMouseEvent);
    subscribe(H_("input.keyboard"), handler, &CameraController::onKeyboardEvent);
}

void CameraController::update(const GameClock& clock)
{
    float dt = clock.get_frame_duration();

    // Update camera
    camera_states_[current_state_]->control(camera_, dt);
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
    if(kbd.key_binding == H_("k_cam_new_keyframe"))
    {
        CameraStateTrackingShot* ts_state
            = static_cast<CameraStateTrackingShot*>(camera_states_[CameraStateIndex::TRACKING]);
        ts_state->add_keyframe(camera_->get_position(),
                               math::quat(0.f,camera_->get_pitch(),camera_->get_yaw()));
        return true;
    }
    else if(kbd.key_binding == H_("k_cam_gen_track"))
    {
        CameraStateTrackingShot* ts_state
            = static_cast<CameraStateTrackingShot*>(camera_states_[CameraStateIndex::TRACKING]);
        ts_state->generate_interpolator();
        return true;
    }
    else if(kbd.key_binding == H_("k_cam_next_state"))
    {
        // TMP just advance state index
        current_state_ = (current_state_+1)%camera_states_.size();
        camera_states_[current_state_]->on_load();
        return true;
    }

    // * Propagate events down to current camera state object
    return camera_states_[current_state_]->onKeyboardEvent(data, camera_);
}

bool CameraController::onMouseEvent(const WData& data)
{
    return camera_states_[current_state_]->onMouseEvent(data, camera_);
}

bool CameraStateFreefly::onMouseEvent(const WData& data, std::shared_ptr<Camera> camera)
{
    const MouseData& md = static_cast<const MouseData&>(data);
    camera->update_orientation(md.dx, md.dy);

    return true; // Do NOT consume event
}

bool CameraStateFreefly::onKeyboardEvent(const WData& data, std::shared_ptr<Camera> camera)
{
    const KbdData& kbd = static_cast<const KbdData&>(data);

    switch(kbd.key_binding)
    {
        case H_("k_run"):
            camera->set_speed(Camera::SPEED_FAST);
            break;
        case H_("k_walk"):
            camera->set_speed(Camera::SPEED_SLOW);
            break;
        case H_("k_forward"):
            camera->move_forward();
            break;
        case H_("k_backward"):
            camera->move_backward();
            break;
        case H_("k_strafe_left"):
            camera->strafe_left();
            break;
        case H_("k_strafe_right"):
            camera->strafe_right();
            break;
        case H_("k_ascend"):
            camera->ascend();
            break;
        case H_("k_descend"):
            camera->descend();
            break;
    }

    return true; // Do NOT consume event
}

CameraStateTrackingShot::CameraStateTrackingShot():
position_interpolator_(nullptr),
orientation_interpolator_(nullptr),
t_(0.f),
max_t_(0.f)
{

}

CameraStateTrackingShot::~CameraStateTrackingShot()
{
    if(position_interpolator_)
        delete position_interpolator_;
    if(orientation_interpolator_)
        delete orientation_interpolator_;
}

bool CameraStateTrackingShot::onMouseEvent(const WData& data, std::shared_ptr<Camera> camera)
{
    const MouseData& md = static_cast<const MouseData&>(data);

    return true; // Do NOT consume event
}

bool CameraStateTrackingShot::onKeyboardEvent(const WData& data, std::shared_ptr<Camera> camera)
{
    const KbdData& kbd = static_cast<const KbdData&>(data);

    return true; // Do NOT consume event
}

void CameraStateTrackingShot::control(std::shared_ptr<Camera> camera, float dt)
{
    if(!position_interpolator_ || !orientation_interpolator_) return;

    math::vec3 newpos(position_interpolator_->interpolate(t_));
    //math::vec3 newori(orientation_interpolator_->interpolate(t_).get_euler_angles());

    //camera->set_orientation(newori.x(), newori.y());
    camera->set_position(newpos);

    //dbg
    /*float pitch = camera->get_pitch();
    float yaw = camera->get_yaw();
    std::cout << 0.f << " " << pitch << " " << yaw << " ";
    math::quat q(0.f,pitch,yaw);
    math::vec3 euler(q.get_euler_angles());
    std::cout << euler << std::endl;
    camera->set_orientation(euler.x(),euler.y());*/

    //dbg lerp quats
    int imax=0;
    for(int ii=0; ii<key_frame_parameters_.size();++ii)
    {
        if(key_frame_parameters_[ii]>t_)
        {
            imax = ii;
            break;
        }
    }
    float t_max = key_frame_parameters_[imax];
    float t_min = key_frame_parameters_[imax-1];
    float alpha = (t_-t_min)/(t_max-t_min);
    const math::quat& q1(key_frame_orientations_[imax-1]);
    const math::quat& q2(key_frame_orientations_[imax]);
    math::quat q(math::slerp(q1,q2,alpha));
    math::vec3 euler = q.get_euler_angles();
    /*math::quat q(q1*(1.f-alpha) + q2*alpha);
    math::vec3 euler = q.get_euler_angles();
    std::cout << q << " " << euler << std::endl;*/
    camera->set_orientation(euler.x(),euler.y());

    //std::cout << t_ << " " << newpos << " " << newori << std::endl;

    t_ += 5*dt;
    if(t_ > max_t_)
        t_ = 0.f;
}

void CameraStateTrackingShot::on_load()
{
    t_ = 0.f; // Reset current parameter value
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

    math::quat ori(orientation);
    std::cout << parameter << " " << position << " " << ori.get_euler_angles() << std::endl;
}

void CameraStateTrackingShot::generate_interpolator()
{
    if(key_frame_parameters_.size() == 0) return;

    if(position_interpolator_)
        delete position_interpolator_;
    if(orientation_interpolator_)
        delete orientation_interpolator_;

    position_interpolator_ = new math::CSpline<math::vec3>(key_frame_parameters_,
                                                           key_frame_positions_);
    orientation_interpolator_ = new math::CSpline<math::quat>(key_frame_parameters_,
                                                              key_frame_orientations_,
                                                              {key_frame_orientations_.front(),
                                                               key_frame_orientations_.back()});

    max_t_ = key_frame_parameters_.back();

    /*key_frame_positions_.clear();
    key_frame_orientations_.clear();
    key_frame_parameters_.clear();*/
}


} // namespace wcore
