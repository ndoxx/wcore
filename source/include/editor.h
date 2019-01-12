#ifndef EDITOR_H
#define EDITOR_H

#include <memory>
#include "game_system.h"
#include "ray_caster.h"

namespace wcore
{

class Model;

enum TRANSFORM_REFERENTIAL: uint8_t
{
    PROPER,
    WORLD,
    CAM,
};

enum TRANSFORM_CONSTRAINT: uint8_t
{
    FREE_HANDLE,
    X,
    Y,
    Z
};

class Editor: public GameSystem
{
public:
    Editor();
    ~Editor();

    virtual void init_events(InputHandler& handler) override;

    inline void set_model_selection(std::weak_ptr<Model> pmdl) { model_selection_ = pmdl; }
    inline std::weak_ptr<Model> get_model_selection() const    { return model_selection_; }

    bool onMouseEvent(const WData& data);
    bool onKeyboardEvent(const WData& data);

    void move_selection();

private:
    uint32_t scene_query_index_;
    SceneQueryResult last_scene_query_;
    std::weak_ptr<Model> model_selection_;
    TRANSFORM_REFERENTIAL current_referential_;
    TRANSFORM_CONSTRAINT current_constraint_;
    bool editing_;
    bool track_cursor_;
};

} // namespace wcore

#endif // EDITOR_H
