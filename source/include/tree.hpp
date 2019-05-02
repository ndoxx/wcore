#include <vector>
#include <array>
#include <functional>
#include <iostream>

namespace wcore
{

template<typename T, size_t MAX_CHILD=4>
class Tree
{
public:
    struct Node;
    typedef Node nodeT;

    Tree():
    n_nodes_(0)
    {

    }

    inline const Node& operator[](int index) const { return nodes_[index]; }
    inline       Node& operator[](int index)       { return nodes_[index]; }

    int add_node(const T& data);
    int add_node(T&& data);
    bool set_child(int node, int child_index);

    void traverse_linear(std::function<void(const T& data)> visit) const;

    inline const Node* get_root() const { return &nodes_[0]; }

protected:
    std::vector<Node> nodes_;
    size_t n_nodes_;
};

// For convenience
#define TREE         Tree<T, MAX_CHILD>
#define TREE_ARGLIST typename T, size_t MAX_CHILD


template<TREE_ARGLIST>
struct TREE::Node
{
public:
    friend class TREE;

    Node(const T& data):
    index_(0),
    parent_(0),
    nchildren_(0),
    data(data)
    {

    }

    Node(T&& data):
    index_(0),
    parent_(0),
    nchildren_(0),
    data(std::move(data))
    {

    }

    inline int parent() const         { return parent_; }
    inline int child(int index) const { return children_[index]; }

    const Node* first_node() const { return nchildren_!=0 ? &(*tree_)[children_[0]] : nullptr; }
    const Node* next_sibling() const
    {
        // Root node has no siblings
        if(parent_==index_)
            return nullptr;

        // Find child index of this node in parent node's children array
        const Node& parent = (*tree_)[parent_];
        int ii;
        for(ii=0; ii<parent.nchildren_; ++ii)
        {
            if(parent.children_[ii] == index_) break;
        }

        // Return node at next index if any
        return (ii<parent.nchildren_-1) ? &(*tree_)[parent.children_[ii+1]] : nullptr;
    }

private:
    int index_;
    int parent_;
    int nchildren_;
    std::array<int, MAX_CHILD> children_;
    TREE* tree_;

public:
    T data;
};

template<TREE_ARGLIST>
int TREE::add_node(const T& data)
{
    nodes_.emplace_back(data);
    nodes_[n_nodes_].tree_ = this;
    nodes_[n_nodes_].index_ = n_nodes_;
    ++n_nodes_;
    return nodes_.size() - 1;
}

template<TREE_ARGLIST>
int TREE::add_node(T&& data)
{
    nodes_.emplace_back(std::forward<T>(data));
    nodes_[n_nodes_].tree_ = this;
    nodes_[n_nodes_].index_ = n_nodes_;
    ++n_nodes_;
    return nodes_.size() - 1;
}

template<TREE_ARGLIST>
bool TREE::set_child(int node, int child_index)
{
    Node& current = nodes_[node];
    if(current.nchildren_==MAX_CHILD)
        return false;

    current.children_[current.nchildren_++] = child_index;
    nodes_[child_index].parent_ = node;
    return true;
}

template<TREE_ARGLIST>
void TREE::traverse_linear(std::function<void(const T&)> visit) const
{
    for(int ii=0; ii<n_nodes_; ++ii)
        visit(nodes_[ii].data);
}

#undef TREE
#undef TREE_ARGLIST

} // namespace wcore
