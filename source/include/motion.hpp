#ifndef MOTION_HPP
#define MOTION_HPP

#include "bezier.h"
#include "cspline.h"
#include "model.h"

namespace timeEvolution
{

class TimeUpdater
{
public:
    virtual ~TimeUpdater(){}
    virtual void step(float& time, float dt, float scale, float tmin, float tmax) = 0;
};

class Alternating: public TimeUpdater
{
private:
    bool forward_;

public:
    Alternating(): forward_(true) {}

    virtual void step(float& time, float dt, float scale, float tmin, float tmax) override
    {
        if(forward_)
        {
            time += dt*scale;
            if(time>tmax)
            {
                time = tmax;
                forward_ = false;
            }
        }
        else
        {
            time -= dt*scale;
            if(time<tmin)
            {
                time = tmin;
                forward_ = true;
            }
        }
    }
};

class Cyclic: public TimeUpdater
{
public:
    virtual void step(float& time, float dt, float scale, float tmin, float tmax) override
    {
        time += dt*scale;
        if(time>tmax)
            time = tmin;
    }
};
}

template <typename T>
class Interpolator
{
public:
    virtual T operator()(float t) = 0;
    virtual ~Interpolator(){}
};

class BezierInterpolator : public math::Bezier, public Interpolator<math::vec3>
{
public:
    explicit BezierInterpolator(const std::vector<math::vec3>& controls):
    Bezier(controls)
    {}
    explicit BezierInterpolator(const Bezier& curve):
    Bezier(curve)
    {}

    virtual math::vec3 operator()(float t) override
    {
        return interpolate(t);
    }
};

class PositionUpdater
{
private:
    float t_;

    Interpolator<math::vec3>*  interpolator_;
    timeEvolution::TimeUpdater* time_updater_;

    float scale_;
    float tmin_;
    float tmax_;

    math::vec3& target_;

public:
    PositionUpdater(math::vec3& target,
                    Interpolator<math::vec3>* interpolator,
                    timeEvolution::TimeUpdater* time_updater,
                    float scale = 1.0f,
                    float tmin = 0.0f,
                    float tmax = 1.0f):
    t_(0),
    interpolator_(interpolator),
    time_updater_(time_updater),
    scale_(scale),
    tmin_(tmin),
    tmax_(tmax),
    target_(target){}

    ~PositionUpdater()
    {
        delete interpolator_;
        delete time_updater_;
    }

    inline void set_time(float tt) { t_ = tt; }
    inline float get_time() const  { return t_; }

    inline void set_step_scale(float scale) { scale_ = scale; }
    inline float get_step_scale() const     { return scale_; }

    inline void set_tmin(float tmin)        { tmin_ = tmin; }
    inline float get_tmin() const           { return tmin_; }

    inline void set_tmax(float tmax)        { tmax_ = tmax; }
    inline float get_tmax() const           { return tmax_; }

    void operator()(float dt)
    {
        time_updater_->step(t_, dt, scale_, tmin_, tmax_);
        target_ = (*interpolator_)(t_);
    }
};
using ColorUpdater = PositionUpdater;


class ConstantRotator
{
private:
    pModel target_;
    math::vec3 angular_rate_;

public:
    explicit ConstantRotator(pModel target,
                             const math::vec3& angular_rate):
    target_(target),
    angular_rate_(angular_rate)
    {}

    inline void set_angular_rate(const math::vec3& value) { angular_rate_ = value; }
    inline const math::vec3& get_angular_rate() const { return angular_rate_; }

    void operator()(float dt)
    {
        target_->rotate(dt*angular_rate_.x(),
                        dt*angular_rate_.y(),
                        dt*angular_rate_.z());
    }
};



#endif // MOTION_HPP
