#ifndef ROCK_GENERATOR_H
#define ROCK_GENERATOR_H

#include <cstdint>

#include "noise_generator.hpp"
#include "noise_policy.hpp"
#include "mesh_descriptor.h"

namespace wcore
{

struct RockProps: public MeshDescriptor
{
public:
    uint32_t gen_seed;
    uint32_t seed;
    uint32_t mesh_density;
    uint32_t octaves;
    float  frequency;
    float  persistence;
    float  loBound;
    float  hiBound;
    //float  startX;
    //float  startZ;
    float  scale;

    virtual void parse_xml(rapidxml::xml_node<char>* node) override;
    virtual hash_t hash() override;
};

struct Vertex3P;
struct Vertex3P3N3T2U;
template <typename VertexT> class Mesh;

class RockGenerator
{
public:
    static Mesh<Vertex3P3N3T2U>* generate_rock(const RockProps& props);

private:
    static NoiseGenerator2D<SimplexNoise<>> RNG_simplex_;
    static uint32_t last_seed_;
};

}

MAKE_HASHABLE(wcore::RockProps, t.gen_seed, t.seed, t.mesh_density, t.octaves,
              t.frequency, t.persistence, t.loBound, t.hiBound, t.scale)

#endif // ROCK_GENERATOR_H
