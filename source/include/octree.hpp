#ifndef OCTREE_H
#define OCTREE_H

#include <cstdint>
#include <cassert>
#include <bitset>
#include <functional>
#include <iostream>

#include "math3d.h"
#include "bounding_boxes.h"

namespace wcore
{

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
    typedef std::function<void(const data_t&)> data_visitor_t;

    Octree();
    Octree(const BoundingRegion& bounding_region, content_t&& content);
    ~Octree();

    // Recursive spatial subdivision
    void subdivide(Octree* current=nullptr);

    // Recursive depth first traversal of objects within range
    // Can be used with FrustumBox for visible range traversal
    // RangeT must define a bool intersects() method that works
    // with BoundingRegion AND primitive_t
    template <typename RangeT>
    void traverse_range(const RangeT& query_range,
                        data_visitor_t visit,
                        Octree* current=nullptr);

    // Recursive octree leaves traversal
    void traverse_leaves(data_visitor_t visit, Octree* current=nullptr);

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

    // * Dispatch current content into children octants when possible
    auto it = current->content_.begin();
    while(it != current->content_.end())
    {
        bool remove_object = false;
        for(int ii=0; ii<8; ++ii)
        {
            // If object is within child bounding region, add it to child content
            // otherwise it stays at this level
            if(current->children_[ii].bounding_region_.contains(it->first))
            {
                current->children_[ii].content_.push_back(*it);
                // Mark object for removal
                remove_object = true;
                break;
            }
        }
        // Remove moved object from current content
        if(remove_object)
            current->content_.erase(it++);
        else
            ++it;
    }

    // * Recursively subdivide children nodes
    for(int ii=0; ii<8; ++ii)
        subdivide(&current->children_[ii]);
}

template <OCTREE_TARGS>
void OCTREE::traverse_leaves(data_visitor_t visit, Octree* current)
{
    // * Initial condition
    if(current == nullptr)
        current = this;

    // * Stop condition
    if(current->is_leaf_node())
    {
        for(auto&& data: current->content_)
            visit(data);
        return;
    }

    // * Walk down the octree recursively
    for(int ii=0; ii<8; ++ii)
    {
        traverse_leaves(visit, &current->children_[ii]);
    }
}

template <OCTREE_TARGS>
template <typename RangeT>
void OCTREE::traverse_range(const RangeT& query_range,
                            data_visitor_t visit,
                            Octree* current)
{
    // * Initial condition
    if(current == nullptr)
        current = this;

    // * Stop condition
    // Check bounding region intersection, return if out of range
    if(!query_range.intersects(current->bounding_region_))
        return;

    // * Visit objects within range at this level
    for(auto&& data: current->content_)
        if(query_range.intersects(data.first))
            visit(data);

    // * Walk down the octree recursively
    if(!current->is_leaf_node())
        for(int ii=0; ii<8; ++ii)
            traverse_range(query_range, visit, &current->children_[ii]);
}

} // namespace wcore


#endif // OCTREE_H
