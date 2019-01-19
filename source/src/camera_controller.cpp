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
    CameraMode* dbg_freefly = new CameraModeFreefly();
    camera_modes_.push_back(dbg_freefly);
    current_mode_ = 0;
}

CameraController::~CameraController()
{
    for(CameraMode* cm: camera_modes_)
        delete cm;
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
    // TMP get scene camera -> later, write a register func
    auto pscene = locate<Scene>(H_("Scene"));
    auto pcam = pscene->get_camera();
    pcam->update(dt);
}

bool CameraController::onKeyboardEvent(const WData& data)
{
    // TMP get scene camera -> later, write a register func
    auto pscene = locate<Scene>(H_("Scene"));
    auto pcam = pscene->get_camera();
    return camera_modes_[current_mode_]->onKeyboardEvent(data, pcam);
}

bool CameraController::onMouseEvent(const WData& data)
{
    // TMP get scene camera -> later, write a register func
    auto pscene = locate<Scene>(H_("Scene"));
    auto pcam = pscene->get_camera();
    return camera_modes_[current_mode_]->onMouseEvent(data, pcam);
}

bool CameraModeFreefly::onMouseEvent(const WData& data, std::shared_ptr<Camera> camera)
{
    const MouseData& md = static_cast<const MouseData&>(data);
    camera->update_orientation(md.dx, md.dy);

    return true; // Do NOT consume event
}

bool CameraModeFreefly::onKeyboardEvent(const WData& data, std::shared_ptr<Camera> camera)
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

} // namespace wcore
