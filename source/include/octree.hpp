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
namespace detail
{
template <class PrimitiveT, class UserDataT, uint32_t MAX_CELL_COUNT, uint32_t MAX_DEPTH>
class OctreeNode
{
public:
    struct DataT;
    typedef std::list<DataT> ContentT;
    typedef std::function<void(const DataT&)> DataVisitorT;

    OctreeNode();
    OctreeNode(const BoundingRegion& bounding_region, ContentT&& content);
    ~OctreeNode();

    // Lazy insertion of a single data element
    inline void insert(const DataT& data);

    // Lazy insertion of a data list
    inline void insert(const ContentT& data);
    inline void insert(ContentT&& data);

    // Recursive removal of an object inserted with a given user data
    bool remove(const UserDataT& udata, OctreeNode* current=nullptr);

    // Recursive object dispatch (will make effective any previous insertion)
    void propagate(OctreeNode* current=nullptr);

    // Recursive depth first traversal of objects within specified range
    // Can be used with FrustumBox for visible range traversal
    // RangeT must define a bool intersects() method that works
    // with BoundingRegion AND PrimitiveT
    template <typename RangeT>
    void traverse_range(const RangeT& query_range,
                        DataVisitorT visit,
                        OctreeNode* current=nullptr);

    // Recursive octree leaves traversal
    void traverse_leaves(DataVisitorT visit, OctreeNode* current=nullptr);

    // Find best fit octant for an input primitive
    // There must exist a center() function that returns the center of a PrimitiveT
    inline uint8_t best_fit_octant(const PrimitiveT& primitive);

    inline bool is_leaf_node() const                 { return (children_ == nullptr); }
    inline bool is_root_node() const                 { return (parent_ == nullptr); }
    inline const BoundingRegion& get_bounds() const  { return bounding_region_; }
    inline const ContentT& get_content() const       { return content_; }

private:
    // Subdivide current cell into 8 octants
    void subdivide();

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
    OctreeNode* parent_;               // parent node, nullptr if root node
    OctreeNode** children_;            // array of 8 octants, nullptr if leaf node
    uint32_t depth_;                   // depth at this node
    BoundingRegion bounding_region_;   // represents the volume spanned by this node
    ContentT content_;                 // data container at this node
};

// For convenience
#define OCTREE_NODE         OctreeNode<PrimitiveT, UserDataT, MAX_CELL_COUNT, MAX_DEPTH>
#define OCTREE_NODE_ARGLIST class PrimitiveT, class UserDataT, uint32_t MAX_CELL_COUNT, uint32_t MAX_DEPTH

template <OCTREE_NODE_ARGLIST>
struct OCTREE_NODE::DataT
{
    DataT(const PrimitiveT& p, const UserDataT& d):
    primitive(p),
    data(d),
    is_placed(false){}

    inline bool operator==(const DataT& other)
    {
        return data == other.data;
    }

    PrimitiveT primitive;
    UserDataT data;
    bool is_placed; // object arrived at its correct node and does not need to be propagated lower
};

template <OCTREE_NODE_ARGLIST>
OCTREE_NODE::OctreeNode():
parent_(nullptr),
children_(nullptr),
depth_(0)
{

}

template <OCTREE_NODE_ARGLIST>
OCTREE_NODE::OctreeNode(const BoundingRegion& bounding_region, ContentT&& content):
parent_(nullptr),
children_(nullptr),
depth_(0),
bounding_region_(bounding_region)
{
    content_.splice(content_.end(), content);
}

template <OCTREE_NODE_ARGLIST>
OCTREE_NODE::~OctreeNode()
{
    if(children_)
    {
        for(int ii=0; ii<8; ++ii)
            delete children_[ii];
    }
    delete[] children_;
}

template <OCTREE_NODE_ARGLIST>
inline void OCTREE_NODE::insert(const DataT& data)
{
    content_.push_back(data);
}

template <OCTREE_NODE_ARGLIST>
inline void OCTREE_NODE::insert(const ContentT& data)
{
    content_.insert(content_.end(), data.begin(), data.end());
}

template <OCTREE_NODE_ARGLIST>
inline void OCTREE_NODE::insert(ContentT&& data)
{
    content_.splice(content_.end(), data);
}

template <OCTREE_NODE_ARGLIST>
inline uint8_t OCTREE_NODE::best_fit_octant(const PrimitiveT& primitive)
{
    // Octants are arranged in a binary decision tree fashion
    // We can use this to our advantage
    math::vec3 diff(center(primitive) - bounding_region_.mid_point);
    return (diff.x()<0 ? 0 : 1) + (diff.z()<0 ? 0 : 2) + (diff.y()<0 ? 0 : 4);
}

template <OCTREE_NODE_ARGLIST>
void OCTREE_NODE::subdivide()
{
    // * Allocate children nodes
    children_ = new OCTREE_NODE*[8];

    // * Set children properties
    for(int ii=0; ii<8; ++ii)
    {
        children_[ii] = new OCTREE_NODE;
        children_[ii]->parent_ = this;
        children_[ii]->depth_  = depth_ + 1;
    }

    // * Subdivide bounding region into 8 octants
    const math::vec3& center = bounding_region_.mid_point;

    // Arrange octants according to a binary decision tree :
    // x>x_c ? 1 : 0
    // z>z_c ? 2 : 0
    // y>y_c ? 4 : 0
    // We add up the evaluations of these 3 decisions to get the index

    // Lower octants
    // Octant 0: x<x_c, y<y_c, z<z_c
    children_[0]->bounding_region_ = BoundingRegion({bounding_region_.extent[0], center[0],
                                                     bounding_region_.extent[2], center[1],
                                                     bounding_region_.extent[4], center[2]});
    // Octant 1: x>x_c, y<y_c, z<z_c
    children_[1]->bounding_region_ = BoundingRegion({center[0], bounding_region_.extent[1],
                                                     bounding_region_.extent[2], center[1],
                                                     bounding_region_.extent[4], center[2]});
    // Octant 2: x<x_c, y<y_c, z>z_c
    children_[2]->bounding_region_ = BoundingRegion({bounding_region_.extent[0], center[0],
                                                     bounding_region_.extent[2], center[1],
                                                     center[2], bounding_region_.extent[5]});
    // Octant 3: x>x_c, y<y_c, z>z_c
    children_[3]->bounding_region_ = BoundingRegion({center[0], bounding_region_.extent[1],
                                                     bounding_region_.extent[2], center[1],
                                                     center[2], bounding_region_.extent[5]});

    // Upper octants
    // Octant 4: x<x_c, y>y_c, z<z_c
    children_[4]->bounding_region_ = BoundingRegion({bounding_region_.extent[0], center[0],
                                                     center[1], bounding_region_.extent[3],
                                                     bounding_region_.extent[4], center[2]});
    // Octant 5: x>x_c, y>y_c, z<z_c
    children_[5]->bounding_region_ = BoundingRegion({center[0], bounding_region_.extent[1],
                                                     center[1], bounding_region_.extent[3],
                                                     bounding_region_.extent[4], center[2]});
    // Octant 6: x<x_c, y>y_c, z>z_c
    children_[6]->bounding_region_ = BoundingRegion({bounding_region_.extent[0], center[0],
                                                     center[1], bounding_region_.extent[3],
                                                     center[2], bounding_region_.extent[5]});
    // Octant 7: x>x_c, y>y_c, z>z_c
    children_[7]->bounding_region_ = BoundingRegion({center[0], bounding_region_.extent[1],
                                                     center[1], bounding_region_.extent[3],
                                                     center[2], bounding_region_.extent[5]});
}

template <OCTREE_NODE_ARGLIST>
void OCTREE_NODE::propagate(OctreeNode* current)
{
    // * Initial condition
    if(current == nullptr)
        current = this;

    // * Check if we need to subdivide cell
    if(current->must_subdivide())
    {
        // Invalidate object placement at this level
        for(auto&& obj: current->content_)
            obj.is_placed = false;
        // Subdivide current cell
        current->subdivide();
    }

    // * Propagate data to children nodes if current node is not a leaf, else return
    if(!current->is_leaf_node())
    {
        // Dispatch current content into children octants when possible
        auto it = current->content_.begin();
        while(it != current->content_.end())
        {
            // If object already arrived at destination, skip tests
            if(it->is_placed) continue;
            // Find index of child octant that will best fit our object
            uint8_t best_octant = current->best_fit_octant(it->primitive);
            // If object is within child bounding region, add it to child content
            // otherwise it stays at this level
            if(current->children_[best_octant]->bounding_region_.contains(it->primitive))
            {
                current->children_[best_octant]->content_.push_back(*it);
                // Remove moved object from current content
                current->content_.erase(it++);
            }
            else
            {
                it->is_placed = true;
                ++it;
            }
        }

        // Recursively propagate data to children nodes
        for(int ii=0; ii<8; ++ii)
            propagate(current->children_[ii]);
    }
    else // Stop condition
    {
        // Objects at leaf nodes are definitely where they should be
        for(auto&& obj: current->content_)
            obj.is_placed = true;
        return;
    }
}

template <OCTREE_NODE_ARGLIST>
bool OCTREE_NODE::remove(const UserDataT& udata, OctreeNode* current)
{
    // * Initial condition
    if(current == nullptr)
        current = this;

    // * Try to remove object at this level
    for(auto it = current->content_.begin(); it != current->content_.end(); ++it)
    {
        if(it->data == udata)
        {
            current->content_.erase(it);
            return true;
        }
    }

    // * If we get here, this means the object couldn't be found at this level
    // so try to find the object lower in the tree
    if(!current->is_leaf_node())
    {
        for(int ii=0; ii<8; ++ii)
        {
            if(remove(udata, current->children_[ii]))
            {
                // Check if we can merge children nodes now that the object has been removed
                if(current->must_merge())
                    current->merge();

                return true;
            }
        }
    }

    return false;
}

template <OCTREE_NODE_ARGLIST>
bool OCTREE_NODE::must_merge()
{
    // * Nothing to merge if we are at the bottom of the tree
    if(is_leaf_node())
        return false;

    // * Sum up the size of all children nodes content
    // if below node capacity, we can merge
    uint32_t count = content_.size();
    for(int ii=0; ii<8; ++ii)
    {
        // If child is not a leaf, total count will necessarily exceed node capacity
        if(!children_[ii]->is_leaf_node())
            return false;

        // Add child object count, bail early if total count exceeds node capacity
        count += children_[ii]->content_.size();
        if(count>MAX_CELL_COUNT)
            return false;
    }
    return true;
}

template <OCTREE_NODE_ARGLIST>
void OCTREE_NODE::merge()
{
    // * Pull back children node content and cleanup
    for(int ii=0; ii<8; ++ii)
    {
        content_.splice(content_.end(), children_[ii]->content_);
        delete children_[ii];
    }

    delete[] children_;
    children_ = nullptr;
}

template <OCTREE_NODE_ARGLIST>
void OCTREE_NODE::traverse_leaves(DataVisitorT visit, OctreeNode* current)
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
        traverse_leaves(visit, current->children_[ii]);
    }
}

template <OCTREE_NODE_ARGLIST>
template <typename RangeT>
void OCTREE_NODE::traverse_range(const RangeT& query_range,
                                 DataVisitorT visit,
                                 OctreeNode* current)
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
            traverse_range(query_range, visit, current->children_[ii]);
}

} // namespace detail

/*
    Octree class for spatial partitioning of data.
    PrimitiveT is the type of objects that are checked against cell bounding regions
        -> math::vec3 for a point octree
        -> AABB for AABB octree
        -> ...
    UserDataT is any kind of data that will be carried along with the primitives.
        -> Useful to store references to game objects so they can be querried later on.
        -> Must be a comparable type
*/
template <class PrimitiveT, class UserDataT, uint32_t MAX_CELL_COUNT=16, uint32_t MAX_DEPTH=5>
class Octree
{
private:
    typedef detail::OctreeNode<PrimitiveT, UserDataT, MAX_CELL_COUNT, MAX_DEPTH> OctreeNode;

public:
    typedef typename OctreeNode::DataT DataT;
    typedef typename OctreeNode::ContentT ContentT;
    typedef typename OctreeNode::DataVisitorT DataVisitorT;

    Octree();
    Octree(const BoundingRegion& bounding_region, ContentT&& content);
    ~Octree();

    void insert(const DataT& data);
    void insert(const ContentT& data);
    void insert(ContentT&& data);

    inline bool remove(const UserDataT& udata)      { return root_->remove(udata); }
    inline void propagate()                         { root_->propagate(); }
    inline void traverse_leaves(DataVisitorT visit) { root_->traverse_leaves(visit); }
    template <typename RangeT>
    inline void traverse_range(const RangeT& query_range,
                               DataVisitorT visit)  { root_->traverse_range(query_range, visit); }

private:
    OctreeNode* root_;
};

// For convenience
#define OCTREE         Octree<PrimitiveT, UserDataT, MAX_CELL_COUNT, MAX_DEPTH>
#define OCTREE_ARGLIST class PrimitiveT, class UserDataT, uint32_t MAX_CELL_COUNT, uint32_t MAX_DEPTH

template <OCTREE_NODE_ARGLIST>
OCTREE::Octree()
{
    root_ = new OctreeNode();
}

template <OCTREE_NODE_ARGLIST>
OCTREE::Octree(const BoundingRegion& bounding_region, ContentT&& content)
{
    root_ = new OctreeNode(bounding_region, std::move(content));
}

template <OCTREE_NODE_ARGLIST>
OCTREE::~Octree()
{
    delete root_;
}

template <OCTREE_NODE_ARGLIST>
void OCTREE::insert(const DataT& data)
{
    // * Detect if object lies outside of bounds
    // If so, we need to expand tree in the direction of the outlier
    if(!root_->bounding_region_.contains(data.primitive))
    {
        std::cout << "outlier: " << data.primitive << std::endl;
        // Create parent node with 8 children, one of which is current root node
    }

    root_->insert(data);
}

template <OCTREE_NODE_ARGLIST>
void OCTREE::insert(const ContentT& data)
{
    root_->insert(data);
}

template <OCTREE_NODE_ARGLIST>
void OCTREE::insert(ContentT&& data)
{
    root_->insert(data);
}

} // namespace wcore


#undef OCTREE
#undef OCTREE_NODE
#undef OCTREE_ARGLIST
#undef OCTREE_NODE_ARGLIST

#endif // OCTREE_H
