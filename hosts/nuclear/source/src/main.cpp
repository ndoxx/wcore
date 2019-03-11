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
/*
#include <iostream>
#include <list>
#include <bitset>

#include "octree.hpp"
#include "math3d.h"
#include "camera.h"

using namespace wcore;

struct UData
{
    float value;
    int key;

    bool operator==(const UData& other)
    {
        return key == other.key;
    }
    friend std::ostream& operator<<(std::ostream& stream, const UData& data);
};

std::ostream& operator<<(std::ostream& stream, const UData& data)
{
    stream << data.value;
    return stream;
}*/
/*
int main()
{
    typedef Octree<math::vec3,UData> PointOctree;
    typedef PointOctree::DataT       DataT;
    typedef PointOctree::ContentT    DataList;

    BoundingRegion start_region({-100,0,0,50,0,100});
    BoundingRegion world_region({-100,100,0,100,-100,100});
    DataList data_points;
    DataList remove_list;

    math::srand_vec3(42);
    for(int ii=0; ii<10000; ++ii)
    {
        math::vec3 point(math::random_vec3(world_region.extent));
        UData user_data({point.norm(), ii});
        DataT obj(point,user_data,ii/1000);
        data_points.push_back(obj);
        if(ii<100)
        {
            // The 100 first objects will be removed
            remove_list.push_back(obj);
        }
    }

    // Populate octree
    PointOctree octree(start_region, data_points);
    octree.propagate();

    // Remove some of the points
    for(auto&& rem: remove_list)
    {
        if(!octree.remove(rem.data))
            std::cout << "Couldn't remove" << std::endl;
    }
    octree.remove_group(1); // remove objects from 1000 to 1999


    // Insert out of bounds point, octree will grow
    //octree.insert(DataT(math::vec3(-120,10,40),UData({0,9900})));
    //octree.propagate();

    uint32_t npoints=0;
    octree.traverse_leaves([&](auto&& obj)
    {
        ++npoints;
        //std::cout << "\t" << obj.primitive << " data: " << obj.data << std::endl;
    });

    std::cout << "traverse_leaves(): Recovered " << npoints << " points." << std::endl;

    npoints=0;
    octree.traverse_range(BoundingRegion({-28,0,0,20,0,50}),
    [&](auto&& obj)
    {
        ++npoints;
        //std::cout << "\t" << obj.primitive << " data: " << obj.data << std::endl;
    });

    std::cout << "traverse_range(BR): Recovered " << npoints << " points." << std::endl;

    npoints=0;
    octree.traverse_range(Sphere(math::vec3(-20,10,50),10),
    [&](auto&& obj)
    {
        ++npoints;
        //std::cout << "\t" << obj.primitive << " data: " << obj.data << std::endl;
    });

    std::cout << "traverse_range(Sphere): Recovered " << npoints << " points." << std::endl;

    Camera camera(1024,768);
    camera.update(1/60.0f);

    npoints=0;
    octree.traverse_range(camera.get_frustum_box(),
    [&](auto&& obj)
    {
        ++npoints;
        //std::cout << "\t" << obj.primitive << " data: " << obj.data << std::endl;
    });

    std::cout << "traverse_range(FB): Recovered " << npoints << " points." << std::endl;

    npoints=0;
    octree.traverse_bounds_range(camera.get_frustum_box(),
    [&](auto&& obj)
    {
        ++npoints;
        //std::cout << "\t" << obj.primitive << " data: " << obj.data << std::endl;
    });

    std::cout << "traverse_bounds_range(FB): Traversed " << npoints << " bounds." << std::endl;

    return 0;
}
*/

#include <iostream>

inline float wrap_uv(float uv)
{
    /*uv   = (uv >= 0.f) ? uv : 1.f + uv;
    return (uv <  1.f) ? uv : uv  - 1.f;*/
    return (uv >= 0.f) ? (uv-(long)uv) : 1.f+(uv-(long)uv);
}

int main()
{
    std::cout << wrap_uv(-2.1f) << std::endl;
    std::cout << wrap_uv(-1.1f) << std::endl;
    std::cout << wrap_uv(-0.2f) << std::endl;
    std::cout << wrap_uv(0.f) << std::endl;
    std::cout << wrap_uv(0.5f) << std::endl;
    std::cout << wrap_uv(1.0f) << std::endl;
    std::cout << wrap_uv(1.1f) << std::endl;
    std::cout << wrap_uv(3.1f) << std::endl;
    std::cout << wrap_uv(12.1f) << std::endl;

    return 0;
}
