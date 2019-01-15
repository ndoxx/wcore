#include <random>

#include "rock_generator.h"
#include "mesh_factory.h"
#include "surface_mesh.h"
#include "logger.h"
#include "xml_utils.hpp"
#include "vertex_format.h"

namespace wcore
{

using namespace math;

void RockProps::parse_xml(rapidxml::xml_node<char>* node)
{
    xml::parse_node(node, "GeneratorSeed", gen_seed);
    xml::parse_node(node, "InstanceSeed", seed);
    xml::parse_node(node, "MeshDensity", mesh_density);
    xml::parse_node(node, "Octaves", octaves);
    xml::parse_node(node, "Frequency", frequency);
    xml::parse_node(node, "Persistence", persistence);
    xml::parse_node(node, "LoBound", loBound);
    xml::parse_node(node, "HiBound", hiBound);
    xml::parse_node(node, "Scale", scale);
}

NoiseGenerator2D<SimplexNoise<>> RockGenerator::RNG_simplex_;
uint32_t RockGenerator::last_seed_ = -1;

Mesh<Vertex3P3N3T2U>* RockGenerator::generate_rock(const RockProps& props)
{
    // * RNG stuff
    std::mt19937 rng;
    rng.seed(props.gen_seed);
    // Has seed changed?
    if(props.gen_seed != last_seed_)
    {
        // If so, reinitialize noise generator
        RNG_simplex_.init(rng);
        last_seed_ = props.gen_seed;
    }
    // Compute random 2D start position for simplex noise function
    std::mt19937 rng_inst;
    rng_inst.seed(props.seed);
    std::uniform_real_distribution<float> pos_distrib(0.0f,1000.0f);
    float startX = pos_distrib(rng_inst);
    float startY = pos_distrib(rng_inst);

    // * First, generate an icosphere with suitable mesh density
    Mesh<Vertex3P3N3T2U>* pmesh = factory::make_ico_sphere(props.mesh_density, false);

    // * Deform mesh with periodic simplex noise
    // For each vertex
    pmesh->traverse_vertices_mut([&](Vertex3P3N3T2U& vertex)
    {
        // Get vertex position
        const vec3& pos(vertex.position_);
        // From position, calculate spherical coordinates
        float theta = acos(pos.y());
        float phi   = atan2(pos.z(), pos.x());
        // From spherical coordinates, calculate a mapping for 2d simplex noise
        float YY = 100.0f * theta;
        float XX = 100.0f * phi;
        // Sample noise function using this mapping
        float samp = RNG_simplex_.octave_noise(XX+startX,
                                               YY+startY,
                                               props.octaves,
                                               props.frequency,
                                               props.persistence);
        samp *= (props.hiBound - props.loBound)/2;
        samp += (props.hiBound + props.loBound)/2;
        // Set new coordinates
        vertex.position_ = samp*pos;
    });

    // * Finalize mesh
    pmesh->build_normals_and_tangents();
    pmesh->compute_dimensions();
    return pmesh;
}

}
