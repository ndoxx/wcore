#ifndef OCTREE_H
#define OCTREE_H

#include <cstdint>
#include <cassert>
#include <bitset>
#include <functional>
#include <iostream>

#include "math3d.h"

namespace wcore
{

struct BoundingRegion
{
    BoundingRegion() = default;
    explicit BoundingRegion(math::extent_t&& value):
    extent(std::move(value)),
    mid_point(math::vec3((extent[1]+extent[0])*0.5f,
                         (extent[3]+extent[2])*0.5f,
                         (extent[5]+extent[4])*0.5f)),
    half(math::vec3((extent[1]-extent[0])*0.5f,
                    (extent[3]-extent[2])*0.5f,
                    (extent[5]-extent[4])*0.5f))
    {

    }

    bool intersects(const BoundingRegion& other) const
    {
        if(std::fabs(mid_point[0] - other.mid_point[0]) > (half[0] + other.half[0]) ) return false;
        if(std::fabs(mid_point[1] - other.mid_point[1]) > (half[1] + other.half[1]) ) return false;
        if(std::fabs(mid_point[2] - other.mid_point[2]) > (half[2] + other.half[2]) ) return false;

        // We have an overlap
        return true;
    };

    bool contains(const math::vec3& point) const
    {
        return(point[0] >= extent[0]
            && point[0] <  extent[1]
            && point[1] >= extent[2]
            && point[1] <  extent[3]
            && point[2] >= extent[4]
            && point[2] <  extent[5]);
    }

    math::extent_t extent;
    math::vec3 mid_point;
    math::vec3 half;
};

/*
    Octree class for spatial partitioning of data.
    primitive_t is the type of objects that are checked against cell bounding regions
        -> math::vec3 for a point octree
        -> AABB for AABB octree
        -> ...
    user_data_t is any kind of data that will be carried along with the primitives.
        -> Each node has a container of std::pair<primitive_t,user_data_t>
        -> Useful to store references to game objects so they can be querried later on.
*/
template <class primitive_t, class user_data_t, uint32_t MAX_CELL_COUNT=16, uint32_t MAX_DEPTH=5>
class Octree
{
public:
    typedef std::pair<primitive_t, user_data_t> data_t;
    typedef std::list<data_t> content_t;

    Octree();
    Octree(const BoundingRegion& bounding_region, content_t&& content);
    ~Octree();

    // Recursive spatial subdivision
    void subdivide(Octree* current=nullptr);

    // Recursive depth first traversal of objects within range
    void traverse_range(const BoundingRegion& query_range,
                        std::function<void(const data_t&)> visit,
                        Octree* current=nullptr);

    // Recursive octree leaves traversal
    void traverse_leaves(std::function<void(Octree*)> visit, Octree* current=nullptr);

    inline void set_node_active(uint8_t index)
    {
        assert(index<8 && "Octree::set_node_active() index out of bounds.");
        active_nodes_[index] = 1;
    }
    inline void set_node_inactive(uint8_t index)
    {
        assert(index<8 && "Octree::set_node_inactive() index out of bounds.");
        active_nodes_[index] = 0;
    }
    inline bool is_node_active(uint8_t index) const
    {
        assert(index<8 && "Octree::is_node_active() index out of bounds.");
        return active_nodes_[index];
    }
    inline bool is_leaf_node() const
    {
        return (children_ == nullptr);
    }
    inline const BoundingRegion& get_bounds() const  { return bounding_region_; }
    inline const content_t& get_content() const      { return content_; }

    inline bool can_subdivide() const
    {
        return (content_.size()>MAX_CELL_COUNT && depth_<(MAX_DEPTH-1));
    }

    // Point-octant intersection test
    inline bool octant_contains(uint8_t index, const math::vec3& point);
    // TODO: implement octant_contains() for bounding boxes

private:
    Octree* parent_;                    // parent node, nullptr if root node
    Octree* children_;                  // array of 8 octants, nullptr if leaf node
    std::bitset<8> active_nodes_;       // flags to signify whether corresponding octants are active
    uint32_t depth_;                    // depth at this node
    BoundingRegion bounding_region_;    // represents the volume spanned by this node
    content_t content_;                 // data container at this node
};

// For convenience
#define OCTREE       Octree<primitive_t, user_data_t, MAX_CELL_COUNT, MAX_DEPTH>
#define OCTREE_TARGS class primitive_t, class user_data_t, uint32_t MAX_CELL_COUNT, uint32_t MAX_DEPTH

template <OCTREE_TARGS>
OCTREE::Octree():
parent_(nullptr),
children_(nullptr),
active_nodes_(0x0),
depth_(0)
{

}

template <OCTREE_TARGS>
OCTREE::Octree(const BoundingRegion& bounding_region, content_t&& content):
parent_(nullptr),
children_(nullptr),
active_nodes_(0x0),
depth_(0),
bounding_region_(bounding_region),
content_(std::move(content))
{

}

template <OCTREE_TARGS>
OCTREE::~Octree()
{
    delete[] children_;
    active_nodes_ = 0x0;
}

template <OCTREE_TARGS>
inline bool OCTREE::octant_contains(uint8_t index, const math::vec3& point)
{
    assert(index<8 && "Octree::octant_contains() index out of bounds.");
    // Point has to be within octant range
    return children_[index].bounding_region_.contains(point);
}

template <OCTREE_TARGS>
void OCTREE::subdivide(Octree* current)
{
    // * Initial condition
    if(current == nullptr)
        current = this;

    // * Stop condition
    if(!current->can_subdivide())
        return;

    // * Allocate children nodes
    delete[] current->children_;
    current->children_ = new OCTREE[8];

    // * Set children properties
    for(int ii=0; ii<8; ++ii)
    {
        current->children_[ii].parent_ = current;
        current->children_[ii].depth_  = current->depth_ + 1;
    }

    // * Subdivide bounding region into 8 octants
    const math::vec3& center = current->bounding_region_.mid_point;
    // Upper octants
    // Octant 0: x>x_c, y>y_c, z>z_c
    current->children_[0].bounding_region_ = BoundingRegion({center[0], current->bounding_region_.extent[1],
                                                             center[1], current->bounding_region_.extent[3],
                                                             center[2], current->bounding_region_.extent[5]});
    // Octant 1: x<x_c, y>y_c, z>z_c
    current->children_[1].bounding_region_ = BoundingRegion({current->bounding_region_.extent[0], center[0],
                                                             center[1], current->bounding_region_.extent[3],
                                                             center[2], current->bounding_region_.extent[5]});
    // Octant 2: x>x_c, y>y_c, z<z_c
    current->children_[2].bounding_region_ = BoundingRegion({center[0], current->bounding_region_.extent[1],
                                                             center[1], current->bounding_region_.extent[3],
                                                             current->bounding_region_.extent[4], center[2]});
    // Octant 3: x<x_c, y>y_c, z<z_c
    current->children_[3].bounding_region_ = BoundingRegion({current->bounding_region_.extent[0], center[0],
                                                             center[1], current->bounding_region_.extent[3],
                                                             current->bounding_region_.extent[4], center[2]});
    // Lower octants
    // Octant 4: x>x_c, y<y_c, z>z_c
    current->children_[4].bounding_region_ = BoundingRegion({center[0], current->bounding_region_.extent[1],
                                                             current->bounding_region_.extent[2], center[1],
                                                             center[2], current->bounding_region_.extent[5]});
    // Octant 5: x<x_c, y<y_c, z>z_c
    current->children_[5].bounding_region_ = BoundingRegion({current->bounding_region_.extent[0], center[0],
                                                             current->bounding_region_.extent[2], center[1],
                                                             center[2], current->bounding_region_.extent[5]});
    // Octant 6: x>x_c, y<y_c, z<z_c
    current->children_[6].bounding_region_ = BoundingRegion({center[0], current->bounding_region_.extent[1],
                                                             current->bounding_region_.extent[2], center[1],
                                                             current->bounding_region_.extent[4], center[2]});
    // Octant 7: x<x_c, y<y_c, z<z_c
    current->children_[7].bounding_region_ = BoundingRegion({current->bounding_region_.extent[0], center[0],
                                                             current->bounding_region_.extent[2], center[1],
                                                             current->bounding_region_.extent[4], center[2]});

    // * Dispatch current content into children octants
    for(const data_t& obj: current->content_)
    {
        for(int ii=0; ii<8; ++ii)
        {
            // If object position is within child bounding region, add it to child content
            // TODO: test against bounding boxes:
            // -> Move object to child list iif child bounds are large enough
            // -> Only delete moved objects from the list
            if(current->octant_contains(ii, obj.first))
            {
                current->children_[ii].content_.push_back(obj);
                break;
            }
        }
    }
    // Remove moved objects from list
    current->content_.clear(); // ftm that's all the objects, bc we only support moving points

    // * Recursively subdivide children nodes
    for(int ii=0; ii<8; ++ii)
    {
        subdivide(&current->children_[ii]);
    }
}

template <OCTREE_TARGS>
void OCTREE::traverse_leaves(std::function<void(Octree*)> visit, Octree* current)
{
    // * Initial condition
    if(current == nullptr)
        current = this;

    // * Stop condition
    if(current->is_leaf_node())
    {
        visit(current);
        return;
    }

    // * Walk down the octree recursively
    for(int ii=0; ii<8; ++ii)
    {
        traverse_leaves(visit, &current->children_[ii]);
    }
}

template <OCTREE_TARGS>
void OCTREE::traverse_range(const BoundingRegion& query_range,
                            std::function<void(const data_t&)> visit,
                            Octree* current)
{
    // * Initial condition
    if(current == nullptr)
        current = this;

    // * Stop condition
    // Check bounding region intersection, return if out of range
    if(!current->bounding_region_.intersects(query_range))
        return;

    // * Visit objects within range at this level
    for(auto&& data: current->content_)
        if(query_range.contains(data.first))
            visit(data);

    // * Walk down the octree recursively
    if(!current->is_leaf_node())
        for(int ii=0; ii<8; ++ii)
            traverse_range(query_range, visit, &current->children_[ii]);
}


} // namespace wcore


#endif // OCTREE_H
