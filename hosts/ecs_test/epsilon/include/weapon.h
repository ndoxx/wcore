#ifndef WEAPON_H
#define WEAPON_H

#include <cstdint>
#include <string>
#include <memory>

enum class damage_t: uint32_t
{
    BULLET,
    LASER,
    PLASMA,
    SHOCKWAVE,
    EM
};

// Base class for ship mounted weapons
class Weapon
{
protected:
    // Damage properties
    damage_t damage_type_;          // Type of damage dealt to target
    float shield_damage_amount_;    // Damage magnitude for target's shield
    float hull_damage_amount_;      // Damage magnitude for target's hull
    float range_;                   // Maximum distance reachable by this weapon
    float accuracy_;                // 100 -> surgical, 0 -> random

    // Behavior
    int hp_;                        // Hit points (health) of this weapon
    bool active_;                   // Weapon is currently in use
    bool enabled_;                  // Weapon is usable
    bool ready_;                    // Weapon is ready to fire

    // Strings
    std::string name_;              // In-game weapon name
    std::string description_;       // In-game weapon description text
public:
    Weapon();
    virtual ~Weapon();

    inline void set_name(const std::string& name) { name_ = name; }
    inline const std::string& get_name() const    { return name_; }

    inline void fire()        { active_ = true; }
    inline void hold()        { active_ = false; }
    inline bool ready() const { return ready_; }

    virtual void update(float dt) = 0;
};

typedef std::shared_ptr<Weapon>       pWeapon;
typedef std::shared_ptr<const Weapon> pcWeapon;
typedef std::weak_ptr<Weapon>         wpWeapon;

// Weapons with a refire rate that shoots individual projectiles
class ProjectileWeapon: public Weapon
{
private:
    float refire_period_ms_;        // Time duration to wait between two projectiles
    float hot_counter_;             // Time elapsed since last burst
    float projectile_velocity_;     // Linear speed of projectiles
public:
    ProjectileWeapon();

    virtual void update(float dt) override;
};

// Weapons that shoot continuous beams with DPS
class BeamWeapon: public Weapon
{
private:
    float depletion_;  // Energy depletion per second (weapon operating)
    float recharge_;   // Energy recharge rate (weapon idle)
    float max_energy_; // Maximum amount of energy in weapon energy buffer
    float energy_;     // Current amount of energy in buffer

public:
    BeamWeapon();

    virtual void update(float dt) override;
};

#endif // WEAPON_H
