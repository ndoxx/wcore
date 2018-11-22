#include "weapon.h"
#include "logger.h"


Weapon::Weapon():
damage_type_(damage_t::BULLET),
shield_damage_amount_(1.f),
hull_damage_amount_(1.f),
range_(100.f),
accuracy_(100.f),
hp_(100),
active_(false),
enabled_(true),
ready_(false)
{

}

Weapon::~Weapon()
{

}


ProjectileWeapon::ProjectileWeapon():
Weapon(),
refire_period_ms_(100.f),
hot_counter_(0.f),
projectile_velocity_(100.f)
{

}

void ProjectileWeapon::update(float dt)
{
    // update weapon only if firing and weapon is enabled
    if(active_ && enabled_)
    {
        hot_counter_ += dt*1e3;
        if(hot_counter_ > refire_period_ms_)
        {
            // weapon is cold, can refire
            hot_counter_ = 0.f;
            ready_ = true;
        }
        else
            ready_ = false;
    }
}

BeamWeapon::BeamWeapon():
Weapon(),
depletion_(500.f),
recharge_(50.f),
max_energy_(100.f),
energy_(max_energy_)
{

}

void BeamWeapon::update(float dt)
{
    if(enabled_)
    {
        if(active_)
        {
            energy_ -= depletion_*dt;
            if(energy_>max_energy_/10.f)
                ready_ = true;
            if(energy_<0.f)
            {
                energy_ = 0.f;
                ready_ = false;
                DLOG("<n>" + name_ + "</n> energy buffer <b>depleted</b>.");
            }
        }
        else
        {
            energy_ += recharge_*dt;
            if(energy_>max_energy_)
                energy_ = max_energy_;
        }
    }
}
