#ifndef TREE_GENERATOR_H
#define TREE_GENERATOR_H

#include <cstdint>
#include <memory>
#include "mesh_descriptor.h"

namespace wcore
{

struct TreeProps: public MeshDescriptor
{
public:
    uint32_t seed = 0;
    uint32_t recursion = 3;
    uint32_t max_branch = 3;
    uint32_t min_samples = 3;
    uint32_t max_samples = 10;
    uint32_t min_sections = 3;
    uint32_t max_sections = 20;
    uint32_t max_nodes = 8;

    float node_prob = 0.2f;
    float branch_prob = 0.2f;
    float hindrance = 0.5f;
    float twist = 0.0f;
    float branch_angle = 0.5f;
    float scale_exponent = 2.0f;
    float trunk_radius = 0.1f;
    float radius_exponent = 0.5f;

    virtual void parse_xml(rapidxml::xml_node<char>* node) override;
};

struct Vertex3P;
struct Vertex3P3N3T2U;
template <typename VertexT> class Mesh;
using SurfaceMesh = Mesh<Vertex3P3N3T2U>;

class TreeGenerator
{
public:
    static Mesh<Vertex3P>* generate_spline_tree(const TreeProps& props);
    static std::shared_ptr<SurfaceMesh> generate_tree(const TreeProps& props);
};

}

MAKE_HASHABLE(wcore::TreeProps, t.seed, t.recursion, t.max_branch, t.min_samples,
              t.max_samples, t.min_sections, t.max_sections, t.max_nodes,
              t.node_prob, t.branch_prob, t.hindrance, t.twist, t.branch_angle,
              t.scale_exponent, t.trunk_radius, t.radius_exponent)


#endif // TREE_GENERATOR_H
