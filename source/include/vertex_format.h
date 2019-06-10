#ifndef VERTEX_FORMAT_H_INCLUDED
#define VERTEX_FORMAT_H_INCLUDED

/**
 * Following structs define vertices in several forms depending on
 * the types of attributes we want to use.
 */

#include <ostream>
#include <vector>

#include "math3d.h"

namespace wcore
{

enum class ShaderDataType: uint8_t
{
    Float = 0, Vec2, Vec3, Vec4, Mat3, Mat4, Int, IVec2, IVec3, IVec4
};

struct BufferLayoutElement
{
    BufferLayoutElement();
    BufferLayoutElement(hash_t name, ShaderDataType type, bool normalized=false);

    uint32_t get_component_count() const;

    hash_t name;
    ShaderDataType type;
    uint32_t size;
    uint32_t offset;
    bool normalized;
};

class BufferLayout
{
public:
    BufferLayout(const std::initializer_list<BufferLayoutElement>& elements);
    void compute_offset_and_stride();

    inline std::vector<BufferLayoutElement>::iterator begin()             { return elements_.begin(); }
    inline std::vector<BufferLayoutElement>::iterator end()               { return elements_.end(); }
    inline std::vector<BufferLayoutElement>::const_iterator begin() const { return elements_.begin(); }
    inline std::vector<BufferLayoutElement>::const_iterator end() const   { return elements_.end(); }

    inline uint32_t get_stride() const { return stride_; }

private:
    std::vector<BufferLayoutElement> elements_;
    uint32_t stride_;
};

/**
 * @brief Vertex with position, normal, texCoord and color attributes.
 * @details Suitable if we want lighting, texturing and coloring.
 *
 * @param position position in world coords
 * @param normal local normal
 * @param uv texture coordinates
 * @param color RGBA color
 */
struct Vertex3P3N2U4C
{
public:
    math::vec3 position_;
    math::vec3 normal_;
    math::vec2 uv_;
    math::vec4 color_;

    static BufferLayout Layout;

    inline std::size_t get_pos_hash() const
    {
        return std::hash<math::vec3>()(position_);
    }

    bool operator==(const Vertex3P3N2U4C& other) const
    {
        return position_==other.position_;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Vertex3P3N2U4C& vf)
    {
        stream << "<p" << vf.position_ << "|n" << vf.normal_
               << "|u" << vf.uv_ << "|c" << vf.color_ << ">" << std::endl;
        return stream;
    }
};

struct Vertex3P3N3T2U
{
public:
    math::vec3 position_;
    math::vec3 normal_;
    math::vec3 tangent_;
    math::vec2 uv_;

    static BufferLayout Layout;

    inline std::size_t get_pos_hash() const
    {
        return std::hash<math::vec3>()(position_);
    }

    bool operator==(const Vertex3P3N3T2U& other) const
    {
        return position_==other.position_;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Vertex3P3N3T2U& vf)
    {
        stream << "<p" << vf.position_ << "|n" << vf.normal_ << "|t" << vf.tangent_
               << "|u" << vf.uv_ << ">" << std::endl;
        return stream;
    }
};

struct VertexAnim
{
public:
    math::vec3 position_;
    math::vec3 normal_;
    math::vec3 tangent_;
    math::vec2 uv_;
    math::vec4 weight_;
    math::i32vec4 bone_id_;

    static BufferLayout Layout;

    inline std::size_t get_pos_hash() const
    {
        return std::hash<math::vec3>()(position_);
    }

    bool operator==(const VertexAnim& other) const
    {
        return position_==other.position_;
    }

    friend std::ostream& operator<<(std::ostream& stream, const VertexAnim& vf)
    {
        stream << "<p" << vf.position_ << "|n" << vf.normal_ << "|t" << vf.tangent_
               << "|u" << vf.uv_ << "|w" << vf.weight_ << "|b" << vf.bone_id_ << ">" << std::endl;
        return stream;
    }
};

struct Vertex3P3N2U
{
public:
    math::vec3 position_;
    math::vec3 normal_;
    math::vec2 uv_;

    static BufferLayout Layout;

    inline std::size_t get_pos_hash() const
    {
        return std::hash<math::vec3>()(position_);
    }

    bool operator==(const Vertex3P3N2U& other) const
    {
        return position_==other.position_;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Vertex3P3N2U& vf)
    {
        stream << "<p" << vf.position_ << "|n" << vf.normal_
               << "|u" << vf.uv_ << ">" << std::endl;
        return stream;
    }
};


struct Vertex3P3N
{
public:
    math::vec3 position_;
    math::vec3 normal_;

    static BufferLayout Layout;

    inline std::size_t get_pos_hash() const
    {
        return std::hash<math::vec3>()(position_);
    }

    bool operator==(const Vertex3P3N& other) const
    {
        return position_==other.position_;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Vertex3P3N& vf)
    {
        stream << "<p" << vf.position_ << "|n" << vf.normal_ << ">" << std::endl;
        return stream;
    }
};


struct Vertex3P2U
{
public:
    math::vec3 position_;
    math::vec2 uv_;

    static BufferLayout Layout;

    friend std::ostream& operator<<(std::ostream& stream, const Vertex3P2U& vf)
    {
        stream << "<p" << vf.position_ << "|u" << vf.uv_ << ">" << std::endl;
        return stream;
    }
};

struct Vertex3P
{
public:
    math::vec3 position_;

    static BufferLayout Layout;

    friend std::ostream& operator<<(std::ostream& stream, const Vertex3P& vf)
    {
        stream << "<p" << vf.position_ << ">" << std::endl;
        return stream;
    }
};

struct Vertex3P3C
{
public:
    math::vec3 position_;
    math::vec3 color_;

    static BufferLayout Layout;

    friend std::ostream& operator<<(std::ostream& stream, const Vertex3P3C& vf)
    {
        stream << "<p" << vf.position_ << "|u" << vf.color_ << ">" << std::endl;
        return stream;
    }
};

struct Vertex2P2U
{
public:
    math::vec2 position_;
    math::vec2 uv_;

    static BufferLayout Layout;

    friend std::ostream& operator<<(std::ostream& stream, const Vertex2P2U& vf)
    {
        stream << "<p" << vf.position_ << "|u" << vf.uv_ << ">" << std::endl;
        return stream;
    }
};

}
#endif // VERTEX_FORMAT_H_INCLUDED
