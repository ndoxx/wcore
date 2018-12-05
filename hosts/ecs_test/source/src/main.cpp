#include <vector>
#include <cassert>
#include <iostream>

#include "wcore.h"

// TMP
#include "wcomponent.h"
#include "wentity.h"
#include "basic_components.h"
// TMP

#include "weapon.h"

using namespace wcore;

class WCModel_Stub: public WComponent
{
public:
    WCModel_Stub(): mesh_(0), material_(0) {}
    WCModel_Stub(int mesh, int material): mesh_(mesh), material_(material) {}

    inline int get_mesh() const     { return mesh_; }
    inline int get_material() const { return material_; }

    inline void set_mesh(int mesh)         { mesh_ = mesh; }
    inline void set_material(int material) { material_ = material; }

private:
    int mesh_;
    int material_;
};
REGISTER_COMPONENT(WCModel_Stub);


class WCShipWeaponMountingPoints: public WComponent
{
private:
    std::vector<pWeapon>    weapons_;
    std::vector<math::vec3> barrel_offsets_;
    math::vec3              target_w_;

public:
    WCShipWeaponMountingPoints() {}

    inline void init(std::size_t n_mount_points)
    {
        weapons_.resize(n_mount_points);
        for(std::size_t ii=0; ii<n_mount_points; ++ii)
            weapons_[ii] = nullptr;
    }

    // Get a weak pointer to weapon mounted at given index
    inline wpWeapon get_weapon(std::size_t index)
    {
        assert(index<weapons_.size() && "[Weapon] index out of bounds in call to get_weapon().");
        return weapons_[index];
    }

    // Change weapon at given index
    inline void set_weapon(std::size_t index, pWeapon weapon)
    {
        assert(index<weapons_.size() && "[Weapon] index out of bounds in call to set_weapon().");
        weapons_[index] = weapon;
    }


    inline void set_weapons(std::initializer_list<pWeapon> weapons,
                            std::initializer_list<math::vec3> barrel_offsets)
    {
        weapons_ = weapons;
        barrel_offsets_ = barrel_offsets;
    }

    inline void set_target(const math::vec3& target_w)
    {
        target_w_ = target_w;
    }

    // Fire a given weapon independently
    inline void fire(std::size_t index)
    {
        assert(index<weapons_.size() && "[Weapon] index out of bounds in call to fire().");
        auto&& weapon = weapons_[index];
        if(weapon)
            weapon->fire();
    }

    // Hold fire at given mounting point
    inline void hold(std::size_t index)
    {
        assert(index<weapons_.size() && "[Weapon] index out of bounds in call to hold().");
        auto&& weapon = weapons_[index];
        if(weapon)
            weapon->hold();
    }

    // Fire all weapons
    inline void fire()
    {
        for(auto&& weapon: weapons_)
            weapon->fire();
    }

    // Hold all weapons
    inline void hold()
    {
        for(auto&& weapon: weapons_)
            weapon->hold();
    }

    // Update weapons
    inline void update(float dt)
    {
        for(auto&& weapon: weapons_)
        {
            weapon->update(dt);
            if(weapon->ready())
            {
                // shoot
                std::cout << weapon->get_name() << " shooting." << std::endl;
            }
        }
    }
};
REGISTER_COMPONENT(WCShipWeaponMountingPoints);

using namespace math;
int main(int argc, char** argv)
{
    std::cout << "ECS test application" << std::endl;

    WEntity ship;
    {
        auto ship_transform = ship.add_component<wcore::component::WCTransform>();
        auto ship_model     = ship.add_component<WCModel_Stub>();
        auto ship_mounts    = ship.add_component<WCShipWeaponMountingPoints>();

        pWeapon plasma_gun(new ProjectileWeapon);
        pWeapon em_gun(new ProjectileWeapon);
        pWeapon laser_gun(new BeamWeapon);
        plasma_gun->set_name("XR-42 Plasma cannon");
        em_gun->set_name("Pulsar MkIII");
        laser_gun->set_name("Sunfury MkV");

        //ship_mounts->init(2);
        ship_mounts->set_weapons({plasma_gun, laser_gun, em_gun},
                                 {vec3(-1,0,0),
                                  vec3(0,0,1),
                                  vec3(1,0,0)});

        ship_transform->set_position(vec3(0.5,0.5,0.5));
    }


    // elsewhere
    float timestamp = 0.f;
    const float dt = 1.f/60.f;

    auto ship_mounts = ship.get_component<WCShipWeaponMountingPoints>();
    ship_mounts->set_target(vec3(0,0,10));

    ship_mounts->fire();
    for(int ii=0; ii<15; ++ii)
    {
        timestamp += dt;
        ship_mounts->update(dt);
    }

    ship_mounts->hold();
    for(int ii=0; ii<15; ++ii)
    {
        timestamp += dt;
        ship_mounts->update(dt);
    }
    return 0;
}
