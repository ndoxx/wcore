#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include <vector>
#include <memory>

#include "game_system.h"

namespace wcore
{

struct WData;
class Camera;
class CameraMode
{
public:
    virtual ~CameraMode() = default;
    virtual bool onMouseEvent(const WData& data, std::shared_ptr<Camera> camera) = 0;
    virtual bool onKeyboardEvent(const WData& data, std::shared_ptr<Camera> camera) = 0;
};

class CameraModeFreefly: public CameraMode
{
public:
    virtual bool onMouseEvent(const WData& data, std::shared_ptr<Camera> camera) override;
    virtual bool onKeyboardEvent(const WData& data, std::shared_ptr<Camera> camera) override;
};

class CameraController: public GameSystem
{
public:
    CameraController();
    ~CameraController();

    inline void next_mode() { current_mode_ = (current_mode_+1)%camera_modes_.size(); }
    inline void set_mode(uint32_t mode)
    {
        assert(mode < camera_modes_.size() && "CameraController::set_mode() index out of bounds.");
        current_mode_ = mode;
    }

    virtual void init_events(InputHandler& handler) override;
    virtual void update(const GameClock& clock) override;

    bool onMouseEvent(const WData& data);
    bool onKeyboardEvent(const WData& data);

private:
    std::vector<CameraMode*> camera_modes_;
    uint32_t current_mode_;
};

} // namespace wcore

#endif // CAMERA_CONTROLLER_H
