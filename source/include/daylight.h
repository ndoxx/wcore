#ifndef DAYLIGHT_H
#define DAYLIGHT_H

#include "updatable.h"
#include "cspline.h"
#include "math3d.h"

namespace wcore
{

class RenderPipeline;
class InputHandler;

class DaylightSystem: public Updatable
{
private:
    RenderPipeline& pipeline_;
    bool active_;
    float daytime_; // Time of day in hours
    float minutes_;
    float seconds_;
    float sun_angle_;
    math::vec3 sun_pos_;

    math::CSplineCatmullV3* color_interpolator_;
    math::CSplineCatmullV3* pp_gamma_interpolator_;
    math::CSplineCatmullF*  brightness_interpolator_;
    math::CSplineCatmullF*  pp_saturation_interpolator_;
    math::CSplineCardinalF* pp_fog_density_interpolator_;
    math::CSplineCardinalF* ambient_strength_interpolator_;

public:
    DaylightSystem(RenderPipeline& pipeline);
    ~DaylightSystem();

    void debug_export_splines();

#ifndef __DISABLE_EDITOR__
    void generate_widget();
#endif

    void setup_user_inputs(InputHandler& handler);
    virtual void update(const GameClock& clock) override;

    inline void toggle() { active_ = !active_; }

    inline void set_color_interp(math::CSplineCatmullV3* interp)
    {
        if(color_interpolator_)
            delete color_interpolator_;
        color_interpolator_ = interp;
    }
    inline void set_gamma_interp(math::CSplineCatmullV3* interp)
    {
        if(pp_gamma_interpolator_)
            delete pp_gamma_interpolator_;
        pp_gamma_interpolator_ = interp;
    }
    inline void set_brightness_interp(math::CSplineCatmullF* interp)
    {
        if(brightness_interpolator_)
            delete brightness_interpolator_;
        brightness_interpolator_ = interp;
    }
    inline void set_saturation_interp(math::CSplineCatmullF* interp)
    {
        if(pp_saturation_interpolator_)
            delete pp_saturation_interpolator_;
        pp_saturation_interpolator_ = interp;
    }
    inline void set_fog_density_interp(math::CSplineCardinalF* interp)
    {
        if(pp_fog_density_interpolator_)
            delete pp_fog_density_interpolator_;
        pp_fog_density_interpolator_ = interp;
    }
    inline void set_ambient_strength_interp(math::CSplineCardinalF* interp)
    {
        if(ambient_strength_interpolator_)
            delete ambient_strength_interpolator_;
        ambient_strength_interpolator_ = interp;
    }
};

}

#endif // DAYLIGHT_H
