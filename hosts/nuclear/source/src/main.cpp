#include <iostream>
#include <ostream>
#include <functional>
#include "math3d.h"
#include "gnuplot-iostream.h"

using namespace wcore;
using namespace math;


struct RigidBodyState
{
    math::vec3 position; // Center of gravity position
    math::vec3 velocity; // C.o.G instant velocity
    float mass = 1.f;    // Total object mass

    friend std::ostream& operator<<(std::ostream& stream, const RigidBodyState& state)
    {
        stream << "p: " << state.position << " v: " << state.velocity;
        return stream;
    }
};

typedef std::function<math::vec3 (const math::vec3&, const math::vec3&, float, float)> force_t;

static force_t gravity = [](const math::vec3& pos, const math::vec3& vel, float m, float t)
{
    return m*math::vec3(0.f, -9.81f, 0.f);
};
static force_t drag = [](const math::vec3& pos, const math::vec3& vel, float m, float t)
{
    return -5.0f*vel.norm2()*vel.normalized();
};
static force_t total_force = [](const math::vec3& pos, const math::vec3& vel, float m, float t)
{
    return gravity(pos,vel,m,t) + drag(pos,vel,m,t);
};

// Runge-Kutta-Nyström integrator
RigidBodyState integrate_RKN4(const RigidBodyState& last, force_t force, float t, float dt)
{
    float mass_inv = 1.0f/last.mass;
    math::vec3 k1 = mass_inv * force(last.position, last.velocity, last.mass, t);
    math::vec3 k2 = mass_inv * force(last.position + 0.5f*dt * last.velocity + 0.125f*dt*dt * k1,
                                     last.velocity + 0.5f*dt * k1, last.mass, t + 0.5f*dt);
    math::vec3 k3 = mass_inv * force(last.position + 0.5f*dt * last.velocity + 0.125f*dt*dt * k2,
                                     last.velocity + 0.5f*dt * k2, last.mass, t + 0.5f*dt);
    math::vec3 k4 = mass_inv * force(last.position +      dt * last.velocity +   0.5f*dt*dt * k3,
                                     last.velocity +      dt * k3, last.mass, t +      dt);
    RigidBodyState next;
    next.position = last.position + dt*last.velocity + dt*dt/6.0f * (k1+k2+k3);
    next.velocity = last.velocity + dt/6.0f * (k1 + 2.0f*k2 + 2.0f*k3 + k4);
    next.mass     = last.mass;
    return next;
}

int main()
{
    RigidBodyState body1;
    body1.position = vec3(0,10,0);
    body1.velocity = vec3(2,10,0);
    body1.mass = 10.f;

    std::vector<std::tuple<float, float, float, float>> plot_points;

    float t  = 0.f;
    float dt = 1.0f/60.0f;
    for(int ii=0; ii<60*5; ++ii)
    {
        float x = body1.position.x();
        float y = body1.position.y();

        body1 = integrate_RKN4(body1, total_force, t, dt);
        if(ii==59)
            std::cout << body1 << std::endl;
        t += dt;

        float dx = body1.position.x()-x;
        float dy = body1.position.y()-y;
        plot_points.push_back(std::make_tuple(x,y,dx,dy));
    }

    // Plot
    Gnuplot gp;

    // Don't forget to put "\n" at the end of each line!
    gp << "set xrange [0:1.2]\nset yrange [0:12]\n";
    // '-' means read from stdin.  The send1d() function sends data to gnuplot's stdin.
    gp << "plot '-' with vectors title 'traj'\n";
    gp.send1d(plot_points);

    return 0;
}
