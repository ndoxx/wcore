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
    PrimitiveT is the type of objects that are checked against cell bounding regions
        -> math::vec3
        -> BoundingRegion
        -> Sphere
        -> ...
    UserDataT is any kind of data that will be carried along with the primitives.
        -> Useful to store references to game objects so they can be querried later on.
        -> Must be a comparable type

    The following trait must exist in namespace wcore::traits:
        -> traits::collision<BoundingRegion,PrimitiveT>
    For range traversal within the bounds of a RangeT type, the two following traits must exist:
        -> traits::collision<RangeT,BoundingRegion>
        -> traits::collision<RangeT,PrimitiveT>
*/
template <class PrimitiveT, class UserDataT, uint32_t MAX_CELL_COUNT=16, uint32_t MAX_DEPTH=5>
class Octree
{
protected:
    class OctreeNode;

public:
    typedef typename OctreeNode::DataT DataT;
    typedef typename OctreeNode::ContentT ContentT;
    typedef typename OctreeNode::DataVisitorT DataVisitorT;
    typedef typename OctreeNode::BoundsVisitorT BoundsVisitorT;

    Octree();
    Octree(const BoundingRegion& bounding_region, const ContentT& content);
    ~Octree();

    void insert(const DataT& data);
    void insert(const ContentT& data_list);

    inline void set_root_bounding_region(const BoundingRegion& bounds) { root_->bounding_region_ = bounds; initialized_ = true; }
    inline bool is_initialized() const              { return initialized_; }
    inline bool remove(const UserDataT& udata)      { return root_->remove(udata); }
    inline void remove_group(uint32_t group_id)     { root_->remove_group(group_id); }
    inline void propagate()                         { root_->propagate(); }
    inline void traverse_leaves(DataVisitorT visit) { root_->traverse_leaves(visit); }
    template <typename RangeT>
    inline void traverse_range(const RangeT& query_range,
                               DataVisitorT visit)  { root_->traverse_range(query_range, visit); }
    template <typename RangeT>
    inline void traverse_bounds_range(const RangeT& query_range,
                                      BoundsVisitorT visit)  { root_->traverse_bounds_range(query_range, visit); }

private:
    void grow(uint8_t old_root_index);

private:
    OctreeNode* root_;
    bool initialized_;
};

// For convenience
#define OCTREE         Octree<PrimitiveT, UserDataT, MAX_CELL_COUNT, MAX_DEPTH>
#define OCTREE_ARGLIST class PrimitiveT, class UserDataT, uint32_t MAX_CELL_COUNT, uint32_t MAX_DEPTH


template <OCTREE_ARGLIST>
class OCTREE::OctreeNode
{
public:
    friend class Octree<PrimitiveT, UserDataT, MAX_CELL_COUNT, MAX_DEPTH>;

    struct DataT;
    typedef std::list<DataT> ContentT;
    typedef std::function<void(const DataT&)> DataVisitorT;
    typedef std::function<void(const BoundingRegion&)> BoundsVisitorT;

    OctreeNode();
    OctreeNode(const BoundingRegion& bounding_region);
    ~OctreeNode();

    // Lazy insertion of a single data element
    inline void insert(const DataT& data);

    // Lazy insertion of a data list
    inline void insert(const ContentT& data);
    inline void insert(ContentT&& data);

    // Recursive removal of an object inserted with a given user data
    bool remove(const UserDataT& udata, OctreeNode* current=nullptr);

    // Recursive removal of an object inserted with a given group id
    void remove_group(uint32_t group_id, OctreeNode* current=nullptr);

    // Recursive object dispatch (will make effective any previous insertion)
    void propagate(OctreeNode* current=nullptr, uint32_t current_depth=0);

    // Recursive depth first traversal of objects within specified range
    // Can be used with FrustumBox for visible range traversal
    // The two following traits must exist:
    // - traits::collision<RangeT,BoundingRegion>
    // - traits::collision<RangeT,PrimitiveT>
    template <typename RangeT>
    void traverse_range(const RangeT& query_range,
                        DataVisitorT visit,
                        OctreeNode* current=nullptr);

    template <typename RangeT>
    void traverse_bounds_range(const RangeT& query_range,
                               BoundsVisitorT visit,
                               OctreeNode* current=nullptr);

    // Recursive octree leaves traversal
    void traverse_leaves(DataVisitorT visit, OctreeNode* current=nullptr);

    // Find best fit octant for an input primitive
    // There must exist a center trait (in namespace wcore::traits) for the type PrimitiveT
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
    inline bool must_subdivide(uint32_t depth) const
    {
        return (content_.size()>MAX_CELL_COUNT && depth<(MAX_DEPTH-1) && is_leaf_node());
    }

private:
    OctreeNode* parent_;               // parent node, nullptr if root node
    OctreeNode** children_;            // array of 8 octants, nullptr if leaf node
    BoundingRegion bounding_region_;   // represents the volume spanned by this node
    ContentT content_;                 // data container at this node
};

template <OCTREE_ARGLIST>
struct OCTREE::OctreeNode::DataT
{
    DataT(const PrimitiveT& p, const UserDataT& d, uint32_t g=0):
    primitive(p),
    data(d),
    group_id(g),
    is_placed(false){}

    inline bool operator==(const DataT& other)
    {
        return data == other.data;
    }

    PrimitiveT primitive;
    UserDataT data;
    uint32_t group_id;
    bool is_placed; // object arrived at its correct node and does not need to be propagated lower
};

template <OCTREE_ARGLIST>
OCTREE::OctreeNode::OctreeNode():
parent_(nullptr),
children_(nullptr)
{

}

template <OCTREE_ARGLIST>
OCTREE::OctreeNode::OctreeNode(const BoundingRegion& bounding_region):
parent_(nullptr),
children_(nullptr),
bounding_region_(bounding_region)
{

}

template <OCTREE_ARGLIST>
OCTREE::OctreeNode::~OctreeNode()
{
    if(children_)
    {
        for(int ii=0; ii<8; ++ii)
            delete children_[ii];
    }
    delete[] children_;
}

template <OCTREE_ARGLIST>
inline void OCTREE::OctreeNode::insert(const DataT& data)
{
    content_.push_back(data);
}

template <OCTREE_ARGLIST>
inline void OCTREE::OctreeNode::insert(const ContentT& data)
{
    content_.insert(content_.end(), data.begin(), data.end());
}

template <OCTREE_ARGLIST>
inline void OCTREE::OctreeNode::insert(ContentT&& data)
{
    content_.splice(content_.end(), data);
}

template <OCTREE_ARGLIST>
inline uint8_t OCTREE::OctreeNode::best_fit_octant(const PrimitiveT& primitive)
{
    // Octants are arranged in a binary decision tree fashion
    // We can use this to our advantage
    math::vec3 diff(traits::center<PrimitiveT>::get(primitive) - bounding_region_.mid_point);
    return (diff.x()<0 ? 0 : 1) + (diff.z()<0 ? 0 : 2) + (diff.y()<0 ? 0 : 4);
}

template <OCTREE_ARGLIST>
void OCTREE::OctreeNode::subdivide()
{
    // * Allocate children nodes
    children_ = new OCTREE::OctreeNode*[8];

    // * Set children properties
    for(int ii=0; ii<8; ++ii)
    {
        children_[ii] = new OCTREE::OctreeNode;
        children_[ii]->parent_ = this;
    }

    // * Subdivide bounding region into 8 octants
    const math::vec3& center = bounding_region_.mid_point;

    // Arrange octants according to a binary decision tree :
    // x>x_c ? 1 : 0
    // z>z_c ? 2 : 0
    // y>y_c ? 4 : 0
    // We add up the evaluations of these 3 decisions to get the index
    // Conversely we can use binary masking on child indices to determine child extent
    for(uint8_t ii=0; ii<8; ++ii)
    {
        math::vec3 new_half   = 0.5f*bounding_region_.half;
        math::vec3 new_center = center
                              + math::vec3(((ii&1)?1.f:-1.f)*new_half.x(),
                                           ((ii&4)?1.f:-1.f)*new_half.y(),
                                           ((ii&2)?1.f:-1.f)*new_half.z());
        children_[ii]->bounding_region_ = BoundingRegion(new_center, new_half);
    }
}

template <OCTREE_ARGLIST>
void OCTREE::OctreeNode::propagate(OctreeNode* current, uint32_t current_depth)
{
    // * Initial condition
    if(current == nullptr)
        current = this;

    // * Check if we need to subdivide cell
    if(current->must_subdivide(current_depth))
    {
        // Invalidate object placement at this level
        for(auto&& obj: current->content_)
            obj.is_placed = false;
        // Subdivide current cell
        current->subdivide();
    }

    // * Propagate data to children nodes if current node is not a leaf, else return
    if(current->is_leaf_node()) // Stop condition
    {
        // Objects at leaf nodes are definitely where they should be
        for(auto&& obj: current->content_)
            obj.is_placed = true;
        return;
    }
    else
    {
        // Dispatch current content into children octants when possible
        auto it = current->content_.begin();
        while(it != current->content_.end())
        {
            // If object already arrived at destination, skip tests
            //if(it->is_placed) continue;
            // Find index of child octant that will best fit our object
            uint8_t best_octant = current->best_fit_octant(it->primitive);
            // If object is within child bounding region, add it to child content
            // otherwise it stays at this level
            // if(current->children_[best_octant]->bounding_region_.contains(it->primitive))
            if(traits::collision<BoundingRegion,PrimitiveT>::contains(
                current->children_[best_octant]->bounding_region_,
                it->primitive))
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
            propagate(current->children_[ii], current_depth+1);
    }

}

template <OCTREE_ARGLIST>
bool OCTREE::OctreeNode::remove(const UserDataT& udata, OctreeNode* current)
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

template <OCTREE_ARGLIST>
void OCTREE::OctreeNode::remove_group(uint32_t group_id, OctreeNode* current)
{
    // * Initial condition
    if(current == nullptr)
        current = this;

    // * Try to remove objects at this level
    auto it = current->content_.begin();
    while(it != current->content_.end())
    {
        if(it->group_id == group_id)
            current->content_.erase(it++);
        else
            ++it;
    }

    // * Try to remove objects lower in the tree
    if(!current->is_leaf_node())
    {
        for(int ii=0; ii<8; ++ii)
        {
            remove_group(group_id, current->children_[ii]);
        }
        // Check if we can merge children nodes now that the object has been removed
        if(current->must_merge())
            current->merge();
    }
}


template <OCTREE_ARGLIST>
bool OCTREE::OctreeNode::must_merge()
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

template <OCTREE_ARGLIST>
void OCTREE::OctreeNode::merge()
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

template <OCTREE_ARGLIST>
void OCTREE::OctreeNode::traverse_leaves(DataVisitorT visit, OctreeNode* current)
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

template <OCTREE_ARGLIST>
template <typename RangeT>
void OCTREE::OctreeNode::traverse_range(const RangeT& query_range,
                                 DataVisitorT visit,
                                 OctreeNode* current)
{
    // * Initial condition
    if(current == nullptr)
        current = this;

    // * Stop condition
    // Check bounding region intersection, return if out of range
    // if(!query_range.intersects(current->bounding_region_))
    //     return;
    if(!traits::collision<RangeT,BoundingRegion>::intersects(query_range, current->bounding_region_))
        return;

    // * Visit objects within range at this level
    // for(auto&& data: current->content_)
    //     if(query_range.intersects(data.primitive))
    //         visit(data);
    for(auto&& data: current->content_)
        if(traits::collision<RangeT,PrimitiveT>::intersects(query_range, data.primitive))
            visit(data);

    // * Walk down the octree recursively
    if(!current->is_leaf_node())
        for(int ii=0; ii<8; ++ii)
            traverse_range(query_range, visit, current->children_[ii]);
}

template <OCTREE_ARGLIST>
template <typename RangeT>
void OCTREE::OctreeNode::traverse_bounds_range(const RangeT& query_range,
                                        BoundsVisitorT visit,
                                        OctreeNode* current)
{
    // * Initial condition
    if(current == nullptr)
        current = this;

    // * Stop condition
    // Check bounding region intersection, return if out of range
    // if(!query_range.intersects(current->bounding_region_))
    //     return;
    if(!traits::collision<RangeT,BoundingRegion>::intersects(query_range,current->bounding_region_))
        return;

    // * Visit region at this level
    visit(current->bounding_region_);

    // * Walk down the octree recursively
    if(!current->is_leaf_node())
        for(int ii=0; ii<8; ++ii)
            traverse_bounds_range(query_range, visit, current->children_[ii]);
}


template <OCTREE_ARGLIST>
OCTREE::Octree():
initialized_(false)
{
    root_ = new OctreeNode();
}

template <OCTREE_ARGLIST>
OCTREE::Octree(const BoundingRegion& bounding_region, const ContentT& content):
initialized_(true)
{
    root_ = new OctreeNode(bounding_region);
    for(auto&& data: content)
        insert(data);
}

template <OCTREE_ARGLIST>
OCTREE::~Octree()
{
    delete root_;
}

template <OCTREE_ARGLIST>
void OCTREE::grow(uint8_t old_root_index)
{
    // Build new root
    OctreeNode* new_root = new OctreeNode();
    new_root->children_  = new OctreeNode*[8];
    // New root spatial extent can be computed by using binary masking on old_root_index
    math::vec3 new_half(2.0f*root_->bounding_region_.half);
    math::vec3 new_center(root_->bounding_region_.mid_point
                        + math::vec3(((old_root_index&1)?-1.f:1.f)*root_->bounding_region_.half.x(),
                                     ((old_root_index&4)?-1.f:1.f)*root_->bounding_region_.half.y(),
                                     ((old_root_index&2)?-1.f:1.f)*root_->bounding_region_.half.z()));
    new_root->bounding_region_ = BoundingRegion(new_center, new_half);

    // Subdivide new root iteratively, but at index old_root_index insert old root
    // instead of creating a new node
    const math::vec3 child_half = root_->bounding_region_.half;
    for(uint8_t ii=0; ii<8; ++ii)
    {
        if(ii!=old_root_index)
        {
            new_root->children_[ii] = new OctreeNode();
            math::vec3 child_center = new_center
                                  + math::vec3(((ii&1)?1.f:-1.f)*child_half.x(),
                                               ((ii&4)?1.f:-1.f)*child_half.y(),
                                               ((ii&2)?1.f:-1.f)*child_half.z());
            new_root->children_[ii]->bounding_region_ = BoundingRegion(child_center, child_half);
        }
        else
            new_root->children_[ii] = root_;
    }
    root_ = new_root;
}

template <OCTREE_ARGLIST>
void OCTREE::insert(const DataT& data)
{
    // * Detect if object lies outside of bounds
    // If so, we need to expand tree in the direction of the outlier
    uint8_t count = 0;
    //while(!root_->bounding_region_.contains(data.primitive))
    while(!traits::collision<BoundingRegion,PrimitiveT>::contains(
            root_->bounding_region_,
            data.primitive))
    {
        // Create parent node with 8 children, one of which is current root node
        // Compute old root index as a child of nex root
        // I showed that it can be obtained by taking the 7's complement to best_fit_octant()
        uint8_t old_root_index = 7 - root_->best_fit_octant(data.primitive);
        grow(old_root_index);

        // APPROX, should be MAX_DEPTH-current_depth
        // But depth is fucking hard to keep track of
        if(++count>MAX_DEPTH)
        {
            std::cout << "Stopped octree growth, it was going to explode." << std::endl;
            break;
        }
    }

    root_->insert(data);
}

template <OCTREE_ARGLIST>
void OCTREE::insert(const ContentT& data_list)
{
    for(auto&& data: data_list)
        insert(data);
}

} // namespace wcore


#undef OCTREE
#undef OCTREE_ARGLIST

#endif // OCTREE_H
