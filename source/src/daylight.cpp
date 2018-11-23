#include <cmath>

#include "daylight.h"
#include "scene.h"
#include "lights.h"
#include "post_processing_renderer.h"
#include "debug_info.h"
#include "logger.h"
#include "pipeline.h"
#include "input_handler.h"
#include "game_clock.h"

#ifndef __DISABLE_EDITOR__
    #include "imgui/imgui.h"
    #include "gui_utils.h"
#endif

namespace wcore
{

using namespace math;
typedef std::shared_ptr<Light> pLight;

static float SUN_INCLINATION = 85.0f * M_PI/180.0f;
static float MOON_INCLINATION = 70.0f * M_PI/180.0f;

DaylightSystem::DaylightSystem(RenderPipeline& pipeline):
pipeline_(pipeline),
active_(true),
daytime_(10.0),
sun_angle_(0),
sun_pos_(0),
color_interpolator_(nullptr),
pp_gamma_interpolator_(nullptr),
brightness_interpolator_(nullptr),
pp_saturation_interpolator_(nullptr),
pp_fog_density_interpolator_(nullptr),
ambient_strength_interpolator_(nullptr)
{
    // Register debug info fields
    DINFO.register_text_slot(H_("sdiTime"), vec3(0.6,1.0,0.0));
    DINFO.register_text_slot(H_("sdiSun"), vec3(1.0,0.5,0.0));
}

DaylightSystem::~DaylightSystem()
{
    delete color_interpolator_;
    delete brightness_interpolator_;
    delete pp_gamma_interpolator_;
    delete pp_saturation_interpolator_;
    delete pp_fog_density_interpolator_;
    delete ambient_strength_interpolator_;
}

#ifndef __DISABLE_EDITOR__
void DaylightSystem::generate_widget()
{
    ImGui::Begin("Ambient parameters");

    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::CollapsingHeader("Sun/Moon control"))
    {
        ImGui::Checkbox("Real-time update", &active_);
        ImGui::SliderFloat("Day time", &daytime_, 0.0f, 24.0f);
        ImGui::SliderFloat("Sun inclination", &SUN_INCLINATION, 0.0f, M_PI);
        ImGui::SliderFloat("Moon inclination", &MOON_INCLINATION, 0.0f, M_PI);
    }

    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::CollapsingHeader("Directional light control"))
    {
        if(auto dir_light = SCENE.get_directional_light_nc().lock())
        {
            ImGui::SliderFloat("Brightness", &dir_light->get_brightness_nc(), 0.0f, 30.0f);
            ImGui::SliderFloat("Ambient strength", &dir_light->get_ambient_strength_nc(), 0.0f, 1.5f);
            ImGui::ColorEdit3("Color", (float*)&dir_light->get_color_nc());
        }
    }


    ImGui::End();
}
#endif

void DaylightSystem::debug_export_splines()
{
#ifdef __DEBUG_SPLINES__
    float nsteps = 100.0f;
    float extrapolation = 0.0f;
    color_interpolator_->dbg_sample("cspline_color.txt", nsteps, extrapolation);
    brightness_interpolator_->dbg_sample("cspline_brightness.txt", nsteps, extrapolation);
    pp_gamma_interpolator_->dbg_sample("cspline_pp_gamma.txt", nsteps, extrapolation);
    pp_saturation_interpolator_->dbg_sample("cspline_pp_saturation.txt", nsteps, extrapolation);
    pp_fog_density_interpolator_->dbg_sample("cspline_pp_fog_density.txt", nsteps, extrapolation);
    ambient_strength_interpolator_->dbg_sample("cspline_ambient_strength.txt", nsteps, extrapolation);
#endif
}

void DaylightSystem::setup_user_inputs(InputHandler& handler)
{
    /*subscribe(H_("k_tg_daysys"), handler, [&](const WData& data)
    {
        toggle();
    });*/
    handler.register_action(H_("k_tg_daysys"), [&]()
    {
        toggle();
    });
}

void DaylightSystem::update(const GameClock& clock)
{
    if(auto dir_light = SCENE.get_directional_light_nc().lock())
    {
        float dt = clock.get_scaled_frame_duration();
        if(DINFO.active())
        {
            std::stringstream ss;
            ss << "Time: " << floor(daytime_) << "h:"
               << floor(minutes_) << ":" << floor(seconds_);
            DINFO.display(H_("sdiTime"), ss.str());

            ss.str("");
            ss << "Sun: direction= " << sun_pos_;
            DINFO.display(H_("sdiSun"), ss.str());
        }

        if(!active_) return;

        daytime_ += dt/5.0f;
        if(daytime_>=24.0f)
            daytime_ = 0.0f;

        minutes_ = 60.0f*(daytime_-floor(daytime_));
        seconds_ = 60.0f*(minutes_-floor(minutes_));

        // Day
        if(daytime_>=6.0f && daytime_<22.0f)
        {
            sun_angle_ = M_PI * (1.0f-(daytime_-6.0f)/18.0f);
            sun_pos_ = vec3(cos(SUN_INCLINATION),sin(SUN_INCLINATION)*sin(sun_angle_),sin(SUN_INCLINATION)*cos(sun_angle_));
        }
        // Night
        else
        {
            sun_angle_ = M_PI * ((daytime_>=22.0f)?(daytime_-22.0f):(daytime_+2.0f))/8.0f;
            sun_pos_ = vec3(sin(MOON_INCLINATION)*cos(sun_angle_),sin(MOON_INCLINATION)*sin(sun_angle_),cos(MOON_INCLINATION));
        }
        sun_pos_.normalize();
        dir_light->set_position(sun_pos_);

        // Other directional light properties
        dir_light->set_color(color_interpolator_->interpolate(daytime_));
        dir_light->set_brightness(fmax(brightness_interpolator_->interpolate(daytime_),0.f));
        dir_light->set_ambient_strength(ambient_strength_interpolator_->interpolate(daytime_));

        // Post processing variables
        pipeline_.set_pp_gamma(pp_gamma_interpolator_->interpolate(daytime_));
        pipeline_.set_pp_saturation(pp_saturation_interpolator_->interpolate(daytime_));
        pipeline_.set_pp_fog_color(color_interpolator_->interpolate(daytime_));
        pipeline_.set_pp_fog_density(pp_fog_density_interpolator_->interpolate(daytime_));
    }
}

}
