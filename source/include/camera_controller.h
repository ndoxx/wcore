#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include <vector>
#include <memory>

#include "game_system.h"
#include "math3d.h"
#include "cspline.h"
#include "slerp_interpolator.h"

namespace wcore
{

struct WData;
class Camera;

enum CameraStateIndex: std::uint8_t
{
    FREEFLY = 0,
    TRACKING = 1,
    N_STATES
};

class CameraController: public GameSystem
{
public:
    CameraController();
    ~CameraController();

    virtual void init_events(InputHandler& handler) override;
    virtual void update(const GameClock& clock) override;

    void register_camera(std::shared_ptr<Camera> camera);

    bool onMouseEvent(const WData& data);
    bool onKeyboardEvent(const WData& data);

public:
    class CameraState;

private:
    std::vector<CameraState*> camera_states_;
    uint32_t current_state_;
    std::shared_ptr<Camera> camera_;
};

class CameraController::CameraState
{
public:
    virtual ~CameraState() = default;
    virtual bool onMouseEvent(const WData& data, std::shared_ptr<Camera> camera) = 0;
    virtual bool onKeyboardEvent(const WData& data, std::shared_ptr<Camera> camera) = 0;
    virtual void control(std::shared_ptr<Camera> camera, float dt) {}
    virtual void on_load() {}
};

class CameraStateFreefly: public CameraController::CameraState
{
public:
    virtual bool onMouseEvent(const WData& data, std::shared_ptr<Camera> camera) override;
    virtual bool onKeyboardEvent(const WData& data, std::shared_ptr<Camera> camera) override;
};

class CameraStateTrackingShot: public CameraController::CameraState
{
public:
    CameraStateTrackingShot();
    ~CameraStateTrackingShot();

    virtual bool onMouseEvent(const WData& data, std::shared_ptr<Camera> camera) override;
    virtual bool onKeyboardEvent(const WData& data, std::shared_ptr<Camera> camera) override;
    virtual void control(std::shared_ptr<Camera> camera, float dt) override;
    virtual void on_load() override;

    void add_keyframe(const math::vec3& position,
                      const math::quat& orientation);
    void generate_interpolator();

private:
    std::vector<math::vec3>    key_frame_positions_;
    std::vector<math::quat>    key_frame_orientations_;
    std::vector<float>         key_frame_parameters_;
    math::CSpline<math::vec3>* position_interpolator_;
    SlerpInterpolator*         orientation_interpolator_;
    float t_; // Current parameter value
    float max_t_;
    float speed_;
};

} // namespace wcore

#endif // CAMERA_CONTROLLER_H
