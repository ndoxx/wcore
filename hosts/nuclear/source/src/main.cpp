/*#include <iostream>
#include <ostream>
#include <functional>
#include "math3d.h"
#include "quaternion.h"
#include "gnuplot-iostream.h"
#include "logger.h"

using namespace wcore;
using namespace math;


struct RigidBodyState
{
    math::vec3 position;         // Center of gravity position
    math::vec3 velocity;         // C.o.G instant velocity
    math::vec3 angular_velocity; // Spin vector
    math::quat orientation;      // Orientation quaternion
    math::mat3 inertia;          // Inertia matrix
    float mass = 1.f;            // Total object mass

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

static math::mat3 skew_matrix(const math::vec3& a)
{
    return math::mat3(0,   -a[2],  a[1],
                      a[2], 0,    -a[0],
                     -a[1], a[0],  0);
}

vec3 newton_raphson_33(const math::mat3& jacobian, const math::vec3& f)
{
    math::mat3 J_inv;
    math::inverse(jacobian, J_inv);
    return J_inv*f;
}

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

RigidBodyState integrate_semi_implicit_euler(const RigidBodyState& last, force_t force, float t, float dt)
{
    RigidBodyState next;
    // * Integrate C.o.G velocity/position
    next.velocity = last.velocity + dt/last.mass * force(last.position, last.velocity, last.mass, t);
    next.position = last.position + dt * next.velocity;
    next.mass     = last.mass;

    // * Integrate orientation
    // Convert to local coordinates (inertia constant in local frame)
    math::vec3 omega_b = last.orientation.rotate_inverse(last.angular_velocity);
    // std::cout << "omega_b " << omega_b << std::endl;
    // Compute residual vector (gyroscopic torque)
    math::vec3 f = dt * math::cross(omega_b, last.inertia * omega_b);
    // std::cout << "f " << f << std::endl;
    // Compute Jacobian for Newton-Raphson step
    math::mat3 J = last.inertia + dt * (skew_matrix(omega_b)*last.inertia - skew_matrix(last.inertia*omega_b));
    // std::cout << "J " << J << std::endl;
    // Single Newton-Raphson step
    omega_b -= newton_raphson_33(J, f);
    // std::cout << "omega_b " << omega_b << std::endl;
    // Get back to world coordinates
    next.angular_velocity = last.orientation.rotate(omega_b);
    // std::cout << "omega " << next.angular_velocity << std::endl;
    // Update quaternion
    next.orientation = last.orientation + 0.5f*dt * math::quat(math::vec4(omega_b));
    next.orientation.normalize();
    // std::cout << "q1 " << next.orientation << std::endl;
    next.inertia = last.inertia;
    return next;
}

int main()
{
    RigidBodyState body1;
    body1.position = vec3(0,10,0);
    body1.velocity = vec3(2,10,0);
    body1.angular_velocity = vec3(-0.5,1,10);
    body1.mass = 10.f;
    body1.inertia = mat3(0.1, 0.5,  0,
                         0.5, 0.1,  0.2,
                         0,   0.2, 0);

    std::vector<std::tuple<float, float, float, float>> plot_points;

    float t  = 0.f;
    float dt = 1.0f/60.0f;
    for(int ii=0; ii<60; ++ii)
    {
        // BANG();
        float x = body1.position.x();
        float y = body1.position.y();

        //body1 = integrate_RKN4(body1, total_force, t, dt);
        body1 = integrate_semi_implicit_euler(body1, total_force, t, dt);
        //std::cout << body1 << std::endl;
        math::vec3 L(body1.inertia * body1.angular_velocity);
        std::cout << L << std::endl;
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
*/

#include <iostream>
#include <list>

#include "octree.hpp"
#include "math3d.h"

using namespace wcore;

struct OctreeContent
{
    typedef math::vec3 entry_t;

    inline void add(const entry_t& entry)
    {
        points_.push_back(entry);
    }
    inline void clear()
    {
        points_.clear();
    }
    inline uint32_t size() const
    {
        return points_.size();
    }

    void traverse(std::function<void(const entry_t&, const math::vec3&)> visit)
    {
        for(auto&& point: points_)
            visit(point, point);
    }

    std::list<math::vec3> points_;
};

int main()
{
    BoundingRegion region({-100,100,0,10,-100,100});
    OctreeContent content;

    math::srand_vec3(0);
    for(int ii=0; ii<1000; ++ii)
    {
        content.points_.push_back(math::random_vec3(region.extent));
    }

    const float    MIN_CELL_SIZE  = 5.0f;
    const uint32_t MAX_CELL_COUNT = 20;

    Octree<OctreeContent> octree(region, std::move(content));
    octree.subdivide([&](const OctreeContent& cur_content, const BoundingRegion& cur_region)
    {
        float size_x = cur_region.extent[1]-cur_region.extent[0];
        return (size_x > MIN_CELL_SIZE)
            && (cur_content.size()>MAX_CELL_COUNT);
    });

    uint32_t npoints=0;
    octree.traverse_leaves([&](Octree<OctreeContent>* leaf)
    {
        std::cout << "--- Current region: ";
        for(int jj=0; jj<6; ++jj)
        {
            std::cout << leaf->get_bounds().extent[jj] << " ";
        }
        std::cout << std::endl;
        std::cout << "\tContent: " << std::endl;
        auto&& content = leaf->get_content();
        for(auto&& point: content.points_)
        {
            ++npoints;
            std::cout << "\t" << point << std::endl;
        }
    });

    std::cout << "Recovered " << npoints << " points." << std::endl;

    return 0;
}
