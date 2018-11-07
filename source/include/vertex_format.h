#ifndef VERTEX_FORMAT_H_INCLUDED
#define VERTEX_FORMAT_H_INCLUDED

/**
 * Following structs define vertices in several forms depending on
 * the types of attributes we want to use.
 */

#include <ostream>
#include <GL/glew.h>

#include "math3d.h"

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
    Vertex3P3N2U4C(const math::vec3& position,
                   const math::vec3& normal,
                   const math::vec2& uv,
                   const math::vec4& color)
    : position_(position)
    , normal_(normal)
    , uv_(uv)
    , color_(color) {}

    Vertex3P3N2U4C(math::vec3&& position,
                   math::vec3&& normal,
                   math::vec2&& uv,
                   math::vec4&& color)
    : position_(std::move(position))
    , normal_(std::move(normal))
    , uv_(std::move(uv))
    , color_(std::move(color)) {}

    inline void set_position(const math::vec3& value) { position_=value; }
    inline void set_normal(const math::vec3& value)   { normal_=value; }
    inline void set_uv(const math::vec2& value)       { uv_=value; }
    inline void set_color(const math::vec4& value)    { color_=value; }
    inline void set_position(math::vec3&& value)      { std::swap(position_, value); }
    inline void set_normal(math::vec3&& value)        { std::swap(normal_, value); }
    inline void set_uv(math::vec2&& value)            { std::swap(uv_, value); }
    inline void set_color(math::vec4&& value)         { std::swap(color_, value); }

    inline const math::vec3& get_position() const { return position_; }
    inline const math::vec3& get_normal()   const { return normal_; }
    inline const math::vec2& get_uv()       const { return uv_; }
    inline const math::vec4& get_color()    const { return color_; }

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

private:
    math::vec3 position_;
    math::vec3 normal_;
    math::vec2 uv_;
    math::vec4 color_;

public:
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
    Vertex3P3N3T2U(const math::vec3& position,
                   const math::vec3& normal,
                   const math::vec3& tangent,
                   const math::vec2& uv)
    : position_(position)
    , normal_(normal)
    , tangent_(tangent)
    , uv_(uv) {}

    Vertex3P3N3T2U(const math::vec3& position,
                   const math::vec2& uv)
    : position_(position)
    , normal_()
    , tangent_()
    , uv_(uv) {}

    Vertex3P3N3T2U(math::vec3&& position,
                   math::vec3&& normal,
                   math::vec3&& tangent,
                   math::vec2&& uv)
    : position_(std::move(position))
    , normal_(std::move(normal))
    , tangent_(std::move(tangent))
    , uv_(std::move(uv)) {}

    Vertex3P3N3T2U(math::vec3&& position,
                   math::vec2&& uv)
    : position_(std::move(position))
    , normal_()
    , tangent_()
    , uv_(std::move(uv)) {}

    inline void set_position(const math::vec3& value) { position_=value; }
    inline void set_normal(const math::vec3& value)   { normal_=value; }
    inline void set_tangent(const math::vec3& value)  { tangent_=value; }
    inline void set_uv(const math::vec2& value)       { uv_=value; }
    inline void set_position(math::vec3&& value)      { std::swap(position_, value); }
    inline void set_normal(math::vec3&& value)        { std::swap(normal_, value); }
    inline void set_tangent(math::vec3&& value)       { std::swap(tangent_, value); }
    inline void set_uv(math::vec2&& value)            { std::swap(uv_, value); }

    inline const math::vec3& get_position() const { return position_; }
    inline const math::vec3& get_normal()   const { return normal_; }
    inline const math::vec3& get_tangent()  const { return tangent_; }
    inline const math::vec2& get_uv()       const { return uv_; }

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

private:
    math::vec3 position_;
    math::vec3 normal_;
    math::vec3 tangent_;
    math::vec2 uv_;

public:
    friend std::ostream& operator<<(std::ostream& stream, const Vertex3P3N3T2U& vf)
    {
        stream << "<p" << vf.position_ << "|n" << vf.normal_ << "|t" << vf.tangent_
               << "|u" << vf.uv_ << ">" << std::endl;
        return stream;
    }
};


struct Vertex3P3N2U
{
public:
    Vertex3P3N2U(const math::vec3& position,
                 const math::vec3& normal,
                 const math::vec2& uv)
    : position_(position)
    , normal_(normal)
    , uv_(uv) {}

    Vertex3P3N2U(math::vec3&& position,
                 math::vec3&& normal,
                 math::vec2&& uv)
    : position_(std::move(position))
    , normal_(std::move(normal))
    , uv_(std::move(uv)) {}

    inline void set_position(const math::vec3& value) { position_=value; }
    inline void set_normal(const math::vec3& value)   { normal_=value; }
    inline void set_uv(const math::vec2& value)       { uv_=value; }
    inline void set_position(math::vec3&& value)      { std::swap(position_, value); }
    inline void set_normal(math::vec3&& value)        { std::swap(normal_, value); }
    inline void set_uv(math::vec2&& value)            { std::swap(uv_, value); }

    inline const math::vec3& get_position() const { return position_; }
    inline const math::vec3& get_normal()   const { return normal_; }
    inline const math::vec2& get_uv()       const { return uv_; }

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

private:
    math::vec3 position_;
    math::vec3 normal_;
    math::vec2 uv_;

public:
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
    Vertex3P3N(const math::vec3& position,
               const math::vec3& normal)
    : position_(position)
    , normal_(normal) {}

    Vertex3P3N(math::vec3&& position,
               math::vec3&& normal)
    : position_(std::move(position))
    , normal_(std::move(normal)) {}

    inline void set_position(const math::vec3& value) { position_=value; }
    inline void set_normal(const math::vec3& value)   { normal_=value; }
    inline void set_position(math::vec3&& value)      { std::swap(position_, value); }
    inline void set_normal(math::vec3&& value)        { std::swap(normal_, value); }

    inline const math::vec3& get_position() const { return position_; }
    inline const math::vec3& get_normal()   const { return normal_; }

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

private:
    math::vec3 position_;
    math::vec3 normal_;

public:
    friend std::ostream& operator<<(std::ostream& stream, const Vertex3P3N& vf)
    {
        stream << "<p" << vf.position_ << "|n" << vf.normal_ << ">" << std::endl;
        return stream;
    }
};


struct Vertex3P2U
{
public:
    Vertex3P2U(const math::vec3& position,
               const math::vec2& uv)
    : position_(position)
    , uv_(uv) {}

    Vertex3P2U(math::vec3&& position,
               math::vec2&& uv)
    : position_(std::move(position))
    , uv_(std::move(uv)) {}

    inline void set_position(const math::vec3& value) { position_=value; }
    inline void set_uv(const math::vec2& value)       { uv_=value; }
    inline void set_position(math::vec3&& value)      { std::swap(position_, value); }
    inline void set_uv(math::vec2&& value)            { std::swap(uv_, value); }

    inline const math::vec3& get_position() const { return position_; }
    inline const math::vec2& get_uv()       const { return uv_; }

    static void enable_vertex_attrib_array()
    {
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P2U), nullptr);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3P2U), (const GLvoid*)(3*sizeof(float)));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
    }

private:
    math::vec3 position_;
    math::vec2 uv_;

public:
    friend std::ostream& operator<<(std::ostream& stream, const Vertex3P2U& vf)
    {
        stream << "<p" << vf.position_ << "|u" << vf.uv_ << ">" << std::endl;
        return stream;
    }
};

struct Vertex3P
{
public:
    Vertex3P(const math::vec3& position)
    : position_(position){}

    Vertex3P(math::vec3&& position)
    : position_(std::move(position)){}

    inline void set_position(const math::vec3& value) { position_=value; }
    inline void set_position(math::vec3&& value)      { std::swap(position_, value); }

    inline const math::vec3& get_position() const { return position_; }

    static void enable_vertex_attrib_array()
    {
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P), nullptr);

        glEnableVertexAttribArray(0);
    }

private:
    math::vec3 position_;

public:
    friend std::ostream& operator<<(std::ostream& stream, const Vertex3P& vf)
    {
        stream << "<p" << vf.position_ << std::endl;
        return stream;
    }
};

struct Vertex2P2U
{
public:
    Vertex2P2U(const math::vec2& position,
               const math::vec2& uv)
    : position_(position)
    , uv_(uv) {}

    Vertex2P2U(math::vec2&& position,
               math::vec2&& uv)
    : position_(std::move(position))
    , uv_(std::move(uv)) {}


    inline void set_position(const math::vec2& value) { position_=value; }
    inline void set_uv(const math::vec2& value)       { uv_=value; }
    inline void set_position(math::vec2&& value)      { std::swap(position_, value); }
    inline void set_uv(math::vec2&& value)            { std::swap(uv_, value); }


    inline const math::vec2& get_position() const { return position_; }
    inline const math::vec2& get_uv()       const { return uv_; }

    static void enable_vertex_attrib_array()
    {
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2P2U), nullptr);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2P2U), (const GLvoid*)(2*sizeof(float)));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
    }

private:
    math::vec2 position_;
    math::vec2 uv_;

public:
    friend std::ostream& operator<<(std::ostream& stream, const Vertex2P2U& vf)
    {
        stream << "<p" << vf.position_ << "|u" << vf.uv_ << ">" << std::endl;
        return stream;
    }
};

#endif // VERTEX_FORMAT_H_INCLUDED
