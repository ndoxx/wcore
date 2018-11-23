#ifndef ROCK_GENERATOR_H
#define ROCK_GENERATOR_H

#include <cstdint>

#include "noise_generator.hpp"
#include "noise_policy.hpp"

namespace rapidxml
{
    template<class Ch> class xml_node;
}

namespace wcore
{

struct RockProps
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

    void parse_xml(rapidxml::xml_node<char>* node);
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

#endif // ROCK_GENERATOR_H
