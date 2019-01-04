#ifndef OCTREE_H
#define OCTREE_H

#include <cstdint>
#include <cassert>
#include <bitset>
#include <functional>
#include <list>
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
    struct data_t
    {
        data_t(const primitive_t& p, const user_data_t& d):
        primitive(p),
        data(d){}

        bool operator==(const data_t& other)
        {
            return data == other.data;
        }

        primitive_t primitive;
        user_data_t data;
    };

    typedef std::list<data_t> content_t;
    typedef std::function<void(const data_t&)> data_visitor_t;

    Octree();
    Octree(const BoundingRegion& bounding_region, content_t&& content);
    ~Octree();

    // Lazy insertion of a single data element
    inline void insert(const data_t& data);

    // Lazy insertion of a data list
    inline void insert(const content_t& data);
    inline void insert(content_t&& data);

    // Remove an object that was inserted with a given user data
    // user_data_t must be comparable
    inline bool remove(const user_data_t& udata);

    // Recursive object dispatch (will make effective any previous insertion)
    void propagate(Octree* current=nullptr);

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
    inline bool is_leaf_node() const                 { return (children_ == nullptr); }
    inline const BoundingRegion& get_bounds() const  { return bounding_region_; }
    inline const content_t& get_content() const      { return content_; }

private:
    // Subdivide current cell into 8 octants
    void subdivide();

    // Recursive removal of an object inserted with a given user data
    bool sub_remove(const user_data_t& udata, Octree* current=nullptr);

    // Merge children nodes and pull back their content at this level
    void merge();

    // Check if we can merge children nodes
    bool must_merge();

    // Only subdivide leaves when they exceed object capacity and are above max depth
    inline bool must_subdivide() const
    {
        return (content_.size()>MAX_CELL_COUNT && depth_<(MAX_DEPTH-1) && is_leaf_node());
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
bounding_region_(bounding_region)
{
    content_.splice(content_.end(), content);
}

template <OCTREE_TARGS>
OCTREE::~Octree()
{
    delete[] children_;
    active_nodes_ = 0x0;
}

template <OCTREE_TARGS>
inline void OCTREE::insert(const data_t& data)
{
    content_.push_back(data);
}
template <OCTREE_TARGS>
inline void OCTREE::insert(const content_t& data)
{
    content_.insert(content_.end(), data.begin(), data.end());
}
template <OCTREE_TARGS>
inline void OCTREE::insert(content_t&& data)
{
    content_.splice(content_.end(), data);
}

template <OCTREE_TARGS>
void OCTREE::subdivide()
{
    // * Allocate children nodes
    delete[] children_;
    children_ = new OCTREE[8];

    // * Set children properties
    for(int ii=0; ii<8; ++ii)
    {
        children_[ii].parent_ = this;
        children_[ii].depth_  = depth_ + 1;
    }

    // * Subdivide bounding region into 8 octants
    const math::vec3& center = bounding_region_.mid_point;
    // Upper octants
    // Octant 0: x>x_c, y>y_c, z>z_c
    children_[0].bounding_region_ = BoundingRegion({center[0], bounding_region_.extent[1],
                                                    center[1], bounding_region_.extent[3],
                                                    center[2], bounding_region_.extent[5]});
    // Octant 1: x<x_c, y>y_c, z>z_c
    children_[1].bounding_region_ = BoundingRegion({bounding_region_.extent[0], center[0],
                                                    center[1], bounding_region_.extent[3],
                                                    center[2], bounding_region_.extent[5]});
    // Octant 2: x>x_c, y>y_c, z<z_c
    children_[2].bounding_region_ = BoundingRegion({center[0], bounding_region_.extent[1],
                                                    center[1], bounding_region_.extent[3],
                                                    bounding_region_.extent[4], center[2]});
    // Octant 3: x<x_c, y>y_c, z<z_c
    children_[3].bounding_region_ = BoundingRegion({bounding_region_.extent[0], center[0],
                                                    center[1], bounding_region_.extent[3],
                                                    bounding_region_.extent[4], center[2]});
    // Lower octants
    // Octant 4: x>x_c, y<y_c, z>z_c
    children_[4].bounding_region_ = BoundingRegion({center[0], bounding_region_.extent[1],
                                                    bounding_region_.extent[2], center[1],
                                                    center[2], bounding_region_.extent[5]});
    // Octant 5: x<x_c, y<y_c, z>z_c
    children_[5].bounding_region_ = BoundingRegion({bounding_region_.extent[0], center[0],
                                                    bounding_region_.extent[2], center[1],
                                                    center[2], bounding_region_.extent[5]});
    // Octant 6: x>x_c, y<y_c, z<z_c
    children_[6].bounding_region_ = BoundingRegion({center[0], bounding_region_.extent[1],
                                                    bounding_region_.extent[2], center[1],
                                                    bounding_region_.extent[4], center[2]});
    // Octant 7: x<x_c, y<y_c, z<z_c
    children_[7].bounding_region_ = BoundingRegion({bounding_region_.extent[0], center[0],
                                                    bounding_region_.extent[2], center[1],
                                                    bounding_region_.extent[4], center[2]});
}

template <OCTREE_TARGS>
void OCTREE::propagate(Octree* current)
{
    // * Initial condition
    if(current == nullptr)
        current = this;

    // * Check if we need to subdivide cell
    if(current->must_subdivide())
        current->subdivide(); // Subdivide current cell

    // * Propagate data to children nodes if current node is not a leaf, else return
    if(!current->is_leaf_node())
    {
        // Dispatch current content into children octants when possible
        auto it = current->content_.begin();
        while(it != current->content_.end())
        {
            bool remove_object = false;
            for(int ii=0; ii<8; ++ii)
            {
                // If object is within child bounding region, add it to child content
                // otherwise it stays at this level
                if(current->children_[ii].bounding_region_.contains(it->primitive))
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

        // Recursively propagate data to children nodes
        for(int ii=0; ii<8; ++ii)
            propagate(&current->children_[ii]);
    }
    else // Stop condition
        return;
}

template <OCTREE_TARGS>
inline bool OCTREE::remove(const user_data_t& udata)
{
    return sub_remove(udata);
}

template <OCTREE_TARGS>
bool OCTREE::sub_remove(const user_data_t& udata, Octree* current)
{
    // * Initial condition
    if(current == nullptr)
        current = this;

    // * Try to remove object at this level
    bool obj_removed = false;
    for(auto it = current->content_.begin(); it != current->content_.end(); ++it)
    {
        if(it->data == udata)
        {
            current->content_.erase(it);
            obj_removed = true;
            break;
        }
    }

    // * If we get here, this means the object couldn't be found at this level
    // so try to find the object lower in the tree
    if(!obj_removed && !current->is_leaf_node())
    {
        for(int ii=0; ii<8; ++ii)
        {
            obj_removed = sub_remove(udata, &current->children_[ii]);
            if(obj_removed)
                break;
        }
    }

    // * Check if we can merge children nodes now that the object has been removed
    if(obj_removed && !current->is_leaf_node())
        if(current->must_merge())
            current->merge();

    return obj_removed;
}

template <OCTREE_TARGS>
bool OCTREE::must_merge()
{
    // * Sum up the size of all children nodes content
    // if below node capacity, we can merge
}

template <OCTREE_TARGS>
void OCTREE::merge()
{

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
        if(query_range.intersects(data.primitive))
            visit(data);

    // * Walk down the octree recursively
    if(!current->is_leaf_node())
        for(int ii=0; ii<8; ++ii)
            traverse_range(query_range, visit, &current->children_[ii]);
}

} // namespace wcore


#endif // OCTREE_H
