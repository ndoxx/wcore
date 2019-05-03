#include <random>
#include <limits>

#include "tree_generator.h"
#include "mesh_factory.h"
#include "math3d.h"
#include "mesh.hpp"
#include "surface_mesh.h"
#include "logger.h"
#include "cspline.h"
#include "xml_utils.hpp"
#include "vertex_format.h"

namespace wcore
{

using namespace math;

void TreeProps::parse_xml(rapidxml::xml_node<char>* node)
{
    xml::parse_node(node, "Seed", seed);
    xml::parse_node(node, "Recursion", recursion);
    xml::parse_node(node, "NodeProbability", node_prob);
    xml::parse_node(node, "BranchProbability", branch_prob);
    xml::parse_node(node, "Twist", twist);
    xml::parse_node(node, "MaxBranch", max_branch);
    xml::parse_node(node, "BranchAngle", branch_angle);
    xml::parse_node(node, "BranchHindrance", hindrance);
    xml::parse_node(node, "ScaleExponent", scale_exponent);
    xml::parse_node(node, "TrunkRadius", trunk_radius);
    xml::parse_node(node, "RadiusExponent", radius_exponent);
    xml::parse_node(node, "MinSamples", min_samples);
    xml::parse_node(node, "MaxSamples", max_samples);
    xml::parse_node(node, "MinSections", min_sections);
    xml::parse_node(node, "MaxSections", max_sections);
    xml::parse_node(node, "MaxNodes", max_nodes);
}

static void make_spline_tree_recursive(std::vector<CSplineCatmullV3>& splines,
                                       std::vector<float>& radii,
                                       std::mt19937 rng,
                                       const TreeProps& props,
                                       uint32_t max_level,
                                       uint32_t cur_level = 0)
{
    // * Stop condition
    if(cur_level>=max_level)
        return;

    std::uniform_real_distribution<float> pos_distrib(0.0f,1.0f);
    std::uniform_real_distribution<float> sym_distrib(-1.0f,1.0f);
    std::binomial_distribution<> branch_distrib(props.max_branch, props.branch_prob);

    // * Add new splines to list given a set of rules
    std::vector<CSplineCatmullV3> new_splines;
    std::vector<float> new_radii;

    // * Start: make a trunk
    if(cur_level==0)
    {
        vec3 rand_dir(0.1f*sym_distrib(rng), 1.0f, 0.1f*sym_distrib(rng));
        rand_dir.normalize();

        std::vector<vec3> controls;
        controls.push_back(vec3(0));
        controls.push_back(0.33f*rand_dir + props.twist*vec3(sym_distrib(rng),sym_distrib(rng),sym_distrib(rng)));
        controls.push_back(0.66f*rand_dir + props.twist*vec3(sym_distrib(rng),sym_distrib(rng),sym_distrib(rng)));
        controls.push_back(1.00f*rand_dir + props.twist*vec3(sym_distrib(rng),sym_distrib(rng),sym_distrib(rng)));

        CSplineCatmullV3 trunk({0.0f, 0.33f, 0.66f, 1.0f},
                               controls);
        splines.push_back(trunk);
        radii.push_back(props.trunk_radius);
    }

    // For each spline, create branches
    const float scale = 0.7f/pow(cur_level+1, props.scale_exponent);
    for(uint32_t jj=0; jj<splines.size(); ++jj)
    {
        CSplineCatmullV3& cur_spline = splines[jj];
        float cur_radius = radii[jj];

        for(uint32_t ii=2; ii<props.max_nodes; ++ii)
        {
            // Do we create a node?
            if(pos_distrib(rng)>props.node_prob && ii!=props.max_nodes-1) continue;

            // Start position of child branch is at this node on parent branch
            vec3 start_pos(cur_spline.interpolate(ii*1.0f/props.max_nodes));
            // Estimate a general direction for parent branch
            vec3 parent_next_pos(cur_spline.interpolate((ii+1)*1.0f/props.max_nodes));
            vec3 parent_direction = parent_next_pos-start_pos;
            parent_direction.normalize();

            // Estimate starting radius of children branches at this node
            float node_radius = cur_radius/pow(ii+1, props.radius_exponent);
            float child_radius = node_radius/1.5f;

            // Compute the number of branches up ahead using binomial law
            // If last node, generate the maximum number of branches
            uint32_t n_branches = (ii==props.max_nodes-1)?props.max_branch:branch_distrib(rng);

            // Create a randomly oriented local basis for the node
            vec3 rand_dir(sym_distrib(rng), sym_distrib(rng), sym_distrib(rng));
            rand_dir.normalize();
            vec3 node_y(parent_direction.cross(rand_dir));
            node_y.normalize();
            vec3 node_x(node_y.cross(parent_direction));
            node_x.normalize();

            // Create branches
            for(uint32_t kk=0; kk<n_branches; ++kk)
            {
                // Choose the orthogonal direction more or less randomly according to
                // branch hindrance value. High hindrance will yield a more symmetric repartition.
                float angular_dev = (1.0f-props.hindrance)*pos_distrib(rng);
                float theta = 2*M_PI*(angular_dev + kk/(1.0f*n_branches));
                // Compute direction using node local basis
                vec3 ortho_direction = cos(theta)*node_x + sin(theta)*node_y;
                // Actual child branch direction is a mix between parent direction and the
                // previously computed orthogonal direction, depending on branch angle
                vec3 child_direction = math::lerp(parent_direction,
                                                  ortho_direction,
                                                  props.branch_angle);
                child_direction.normalize();
                // Generate child branch spline given general direction
                std::vector<vec3> controls;
                controls.push_back(start_pos);
                controls.push_back(start_pos + 0.33f*scale*child_direction + scale*props.twist*vec3(sym_distrib(rng),sym_distrib(rng),sym_distrib(rng)));
                controls.push_back(start_pos + 0.66f*scale*child_direction + scale*props.twist*vec3(sym_distrib(rng),sym_distrib(rng),sym_distrib(rng)));
                controls.push_back(start_pos + 1.00f*scale*child_direction + scale*props.twist*vec3(sym_distrib(rng),sym_distrib(rng),sym_distrib(rng)));

                CSplineCatmullV3 new_spline(std::vector<float>{0.0f, 0.33f, 0.66f, 1.0f},
                                            controls);
                // Add child branch to list
                new_splines.push_back(new_spline);
                new_radii.push_back(child_radius);
            }
        }
    }

    // * Recurse over these new splines
    make_spline_tree_recursive(new_splines, new_radii, rng, props, max_level, cur_level+1);

    // * Copy new splines to splines list
    for(uint32_t ii=0; ii<new_splines.size(); ++ii)
    {
        splines.push_back(new_splines[ii]);
        radii.push_back(new_radii[ii]);
    }
}

Mesh<Vertex3P>* TreeGenerator::generate_spline_tree(const TreeProps& props)
{
    // RNG stuff
    std::mt19937 rng;
    rng.seed(props.seed);
    //std::uniform_int_distribution<uint32_t> mesh_seed(0,std::numeric_limits<uint32_t>::max());
    std::uniform_real_distribution<float> var_distrib(-1.0f,1.0f);

    Mesh<Vertex3P>* pmesh = new Mesh<Vertex3P>;

    // Generate whole tree recursively
    std::vector<CSplineCatmullV3> splines;
    std::vector<float> radii;
    const uint32_t max_level = props.recursion;
    make_spline_tree_recursive(splines, radii, rng, props, max_level);

    // Iterate over splines to generate segments
    const uint32_t N_SAMP = 10;
    const float step = 1.0f/N_SAMP;
    for(uint32_t jj=0; jj<splines.size(); ++jj)
    {
        for(uint32_t ii=0; ii<N_SAMP; ++ii)
        {
            pmesh->_push_vertex({splines[jj].interpolate(ii*step)});
            pmesh->_push_vertex({splines[jj].interpolate((ii+1)*step)});
            pmesh->push_line(jj*2*N_SAMP+ii, jj*2*N_SAMP+ii+1);
        }
    }

    return pmesh;
}

std::shared_ptr<SurfaceMesh> TreeGenerator::generate_tree(const TreeProps& props)
{
    // RNG stuff
    std::mt19937 rng;
    rng.seed(props.seed);
    //std::uniform_int_distribution<uint32_t> mesh_seed(0,std::numeric_limits<uint32_t>::max());
    std::uniform_real_distribution<float> var_distrib(-1.0f,1.0f);

    std::shared_ptr<FaceMesh> pmesh(new FaceMesh);

    // From trunk spline, generate whole tree recursively
    std::vector<CSplineCatmullV3> splines;
    std::vector<float> radii;
    const uint32_t max_level = props.recursion;
    make_spline_tree_recursive(splines, radii, rng, props, max_level);

    // Get maximum and minimum radii so as to choose number of samples linearly
    float rmin = std::numeric_limits<float>::max();
    float rmax = std::numeric_limits<float>::min();
    for(uint32_t jj=0; jj<radii.size(); ++jj)
    {
        if(radii[jj] < rmin) rmin = radii[jj];
        if(radii[jj] > rmax) rmax = radii[jj];
    }

    // Iterate over splines to generate segments
    for(uint32_t jj=0; jj<splines.size(); ++jj)
    {
        float rr = (radii[jj]-rmin) / (rmax-rmin); // between 0 and 1
        uint32_t n_samples  = (uint32_t) math::lerp((float)props.min_samples, (float)props.max_samples, rr);
        uint32_t n_sections = (uint32_t) math::lerp((float)props.min_sections, (float)props.max_sections, rr);
        factory::skin_spline(pmesh, splines[jj], n_sections, n_samples, radii[jj], props.radius_exponent);
    }

    pmesh->build_normals_and_tangents();
    pmesh->smooth_normals_and_tangents();
    pmesh->compute_dimensions();

    return static_cast<std::shared_ptr<SurfaceMesh>>(pmesh);
}

}
