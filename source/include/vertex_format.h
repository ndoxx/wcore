#ifndef VERTEX_FORMAT_H_INCLUDED
#define VERTEX_FORMAT_H_INCLUDED

/**
 * Following structs define vertices in several forms depending on
 * the types of attributes we want to use.
 */

#include <ostream>
#include <GL/glew.h>

#include "math3d.h"

namespace wcore
{

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

    inline std::size_t get_pos_hash() const
    {
        return std::hash<math::vec3>()(position_);
    }

    bool operator==(const Vertex3P3N2U4C& other) const
    {
        return position_==other.position_;
    }

    static void enable_vertex_attrib_array()
    {
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N2U4C), nullptr);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N2U4C), (const GLvoid*)(3*sizeof(float)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N2U4C), (const GLvoid*)(6*sizeof(float)));
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N2U4C), (const GLvoid*)(8*sizeof(float)));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
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

    inline std::size_t get_pos_hash() const
    {
        return std::hash<math::vec3>()(position_);
    }

    bool operator==(const Vertex3P3N3T2U& other) const
    {
        return position_==other.position_;
    }

    static void enable_vertex_attrib_array()
    {
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N3T2U), nullptr);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N3T2U), (const GLvoid*)(3*sizeof(float)));
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N3T2U), (const GLvoid*)(6*sizeof(float)));
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N3T2U), (const GLvoid*)(9*sizeof(float)));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
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

    inline std::size_t get_pos_hash() const
    {
        return std::hash<math::vec3>()(position_);
    }

    bool operator==(const VertexAnim& other) const
    {
        return position_==other.position_;
    }

    static void enable_vertex_attrib_array()
    {
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAnim), nullptr);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAnim), (const GLvoid*)(3*sizeof(float)));
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAnim), (const GLvoid*)(6*sizeof(float)));
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VertexAnim), (const GLvoid*)(9*sizeof(float)));
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(VertexAnim), (const GLvoid*)(11*sizeof(float)));
        glVertexAttribIPointer(5, 4, GL_UNSIGNED_BYTE,  sizeof(VertexAnim), (const GLvoid*)(15*sizeof(float)));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glEnableVertexAttribArray(4);
        glEnableVertexAttribArray(5);
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

    inline std::size_t get_pos_hash() const
    {
        return std::hash<math::vec3>()(position_);
    }

    bool operator==(const Vertex3P3N2U& other) const
    {
        return position_==other.position_;
    }

    static void enable_vertex_attrib_array()
    {
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N2U), nullptr);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N2U), (const GLvoid*)(3*sizeof(float)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N2U), (const GLvoid*)(6*sizeof(float)));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
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

    inline std::size_t get_pos_hash() const
    {
        return std::hash<math::vec3>()(position_);
    }

    bool operator==(const Vertex3P3N& other) const
    {
        return position_==other.position_;
    }

    static void enable_vertex_attrib_array()
    {
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N), nullptr);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N), (const GLvoid*)(3*sizeof(float)));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
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

    static void enable_vertex_attrib_array()
    {
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P2U), nullptr);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3P2U), (const GLvoid*)(3*sizeof(float)));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
    }

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

    static void enable_vertex_attrib_array()
    {
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P), nullptr);

        glEnableVertexAttribArray(0);
    }

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

    static void enable_vertex_attrib_array()
    {
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3C), nullptr);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3C), (const GLvoid*)(3*sizeof(float)));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
    }

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

    static void enable_vertex_attrib_array()
    {
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2P2U), nullptr);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2P2U), (const GLvoid*)(2*sizeof(float)));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
    }

    friend std::ostream& operator<<(std::ostream& stream, const Vertex2P2U& vf)
    {
        stream << "<p" << vf.position_ << "|u" << vf.uv_ << ">" << std::endl;
        return stream;
    }
};

}
#endif // VERTEX_FORMAT_H_INCLUDED
