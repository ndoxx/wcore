#ifndef MODEL_H
#define MODEL_H

#include <bitset>
#include <cassert>

#include "transformation.h"
#include "mesh.hpp"
#include "bounding_boxes.h"

namespace wcore
{

struct Vertex3P3N3T2U;
struct Vertex3P;
class Material;

#ifdef __DEBUG__
struct DebugDisplayOptions
{
    enum : uint8_t
    {
        AABB,
        OBB,
        ORIGIN,
        N_OPTIONS,
    };

    std::bitset<3> flags_;

    inline void enable(uint8_t value)
    {
        assert(value < N_OPTIONS && "[DebugDisplayOptions] Index out of bounds.");
        flags_.set(value);
    }
    inline void disable(uint8_t value)
    {
        assert(value < N_OPTIONS && "[DebugDisplayOptions] Index out of bounds.");
        flags_.set(value, 0);
    }
    inline bool is_enabled(uint8_t value)
    {
        assert(value < N_OPTIONS && "[DebugDisplayOptions] Index out of bounds.");
        return flags_[value];
    }
    inline void clear()
    {
        flags_.reset();
    }
};
#endif

class Model
{
protected:
    Mesh<Vertex3P3N3T2U>* pmesh_;
    Material*             pmaterial_;
    Transformation        trans_;
    OBB                   obb_;
    AABB                  aabb_;
    bool                  frustum_cull_;
    bool                  is_dynamic_;
    uint32_t              shadow_cull_face_;

public:
#ifdef __DEBUG__
    DebugDisplayOptions debug_display_opts_;
#endif

    Model(Mesh<Vertex3P3N3T2U>* pmesh, Material* material);
    ~Model();

    inline const Mesh<Vertex3P3N3T2U>& get_mesh() const         { return *pmesh_; }
    inline Mesh<Vertex3P3N3T2U>& get_mesh()                     { return *pmesh_; }
    inline math::mat4 get_model_matrix()                        { return trans_.get_model_matrix(); }
    inline const Transformation& get_transformation() const     { return trans_; }
    inline Transformation& get_transformation()                 { return trans_; }
    inline void set_transformation(const Transformation& trans) { trans_ = trans; }

    inline const Material& get_material() const                 { return *pmaterial_; }
    inline Material& get_material()                             { return *pmaterial_; }

    inline const math::vec3& get_position() const               { return trans_.get_position(); }

    inline void set_dynamic(bool value)                         { is_dynamic_ = value; }
    inline AABB& get_AABB()                                     { if(is_dynamic_) aabb_.update(); return aabb_; }
    inline void update_AABB()                                   { aabb_.update(); }
    inline void set_AABB_offset(const math::vec3& offset)       { aabb_.set_offset(offset); }
    inline OBB& get_OBB()                                       { if(is_dynamic_) obb_.update(); return obb_; }
    inline void update_OBB()                                    { obb_.update(); }
    inline void set_OBB_offset(const math::vec3& offset)        { obb_.set_offset(offset); }
    inline void update_bounding_boxes()                         { update_OBB(); update_AABB(); }

    inline void set_frustum_cull(bool value)                    { frustum_cull_ = value; }
    inline bool can_frustum_cull() const                        { return frustum_cull_; }
    inline void set_shadow_cull_face(uint32_t value)            { shadow_cull_face_ = value; }
    inline bool shadow_cull_face() const                        { return shadow_cull_face_; }

    inline void set_position(const math::vec3& newpos)          { trans_.set_position(newpos); }
    inline void set_orientation(const math::quat& newori)       { trans_.set_orientation(newori); }
    inline void set_scale(float scale)                          { trans_.set_scale(scale); }
    inline void rotate(float phi, float theta, float psi)       { trans_.rotate(phi, theta, psi); }
    inline void rotate(const math::vec3& axis, float angle)     { trans_.rotate(axis, angle); }
    inline void rotate(math::vec3&& axis, float angle)          { trans_.rotate(std::forward<math::vec3>(axis), angle); }
    inline void translate(const math::vec3& increment)          { trans_.translate(increment); }
    inline void translate(float x, float y, float z)            { trans_.translate(x, y, z); }
    inline void translate_x(float x)                            { trans_.translate_x(x); }
    inline void translate_y(float y)                            { trans_.translate_y(y); }
    inline void translate_z(float z)                            { trans_.translate_z(z); }
};

class LineModel
{
private:
    Mesh<Vertex3P>* pmesh_;
    Material*       pmaterial_;
    Transformation  trans_;

public:
    LineModel(Mesh<Vertex3P>* pmesh, Material* material);
    ~LineModel();

    inline const Mesh<Vertex3P>& get_mesh() const { return *pmesh_; }
    inline Mesh<Vertex3P>& get_mesh() { return *pmesh_; }
    inline math::mat4 get_model_matrix()  { return trans_.get_model_matrix(); }
    inline const Transformation& get_transformation() const { return trans_; }
    inline Transformation& get_transformation() { return trans_; }
    inline void set_transformation(const Transformation& trans) { trans_ = trans; }

    inline const Material& get_material() const { return *pmaterial_; }
    inline Material& get_material() { return *pmaterial_; }

    inline const math::vec3& get_position() const { return trans_.get_position(); }

    inline void set_position(const math::vec3& newpos)      { trans_.set_position(newpos); }
    inline void set_orientation(const math::quat& newori)   { trans_.set_orientation(newori); }
    inline void set_scale(float scale)                      { trans_.set_scale(scale); }
    inline void rotate(float phi, float theta, float psi)   { trans_.rotate(phi, theta, psi); }
    inline void rotate(const math::vec3& axis, float angle) { trans_.rotate(axis, angle); }
    inline void rotate(math::vec3&& axis, float angle)      { trans_.rotate(std::forward<math::vec3>(axis), angle); }
    inline void translate(const math::vec3& increment)      { trans_.translate(increment); }
    inline void translate(float x, float y, float z)        { trans_.translate(x, y, z); }
    inline void translate_x(float x)                        { trans_.translate_x(x); }
    inline void translate_y(float y)                        { trans_.translate_y(y); }
    inline void translate_z(float z)                        { trans_.translate_z(z); }
};

typedef std::shared_ptr<Model> pModel;
typedef std::shared_ptr<const Model> pcModel;
typedef std::shared_ptr<LineModel> pLineModel;
typedef std::shared_ptr<const LineModel> pcLineModel;

}

#endif // MODEL_H
