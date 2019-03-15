#ifndef MODEL_H
#define MODEL_H

#include <bitset>
#include <cassert>

#include "transformation.h"
#include "mesh.hpp"
#include "bounding_boxes.h"
#include "logger.h"
namespace wcore
{

struct Vertex3P3N3T2U;
struct Vertex3P;
class Material;

#ifndef __DISABLE_EDITOR__
class Editor;
#endif

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
    inline bool is_enabled(uint8_t value) const
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
    std::shared_ptr<SurfaceMesh> pmesh_;
    Material*             pmaterial_;
    Transformation        trans_;
    OBB                   obb_;
    AABB                  aabb_;
    bool                  frustum_cull_;
    bool                  is_dynamic_;
    bool                  is_terrain_;
    bool                  visible_;
    uint32_t              shadow_cull_face_;

    hash_t reference_;
    bool   has_reference_;

#ifndef __DISABLE_EDITOR__
    // Editor callback to reset its selection when this model is destroyed
    Editor* editor_;
    void (Editor::*selection_reset_) (void);
#endif


public:
#ifdef __DEBUG__
    DebugDisplayOptions debug_display_opts_;
#endif

    Model(std::shared_ptr<SurfaceMesh> pmesh, Material* material);
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
    inline void update_OBB()                                    { obb_.update(get_model_matrix()); }
    inline void update_AABB()                                   { aabb_.update(get_OBB()); }
    inline AABB& get_AABB()                                     { if(is_dynamic_) update_AABB(); return aabb_; }
    inline OBB& get_OBB()                                       { if(is_dynamic_) update_OBB(); return obb_; }
    inline void set_OBB_offset(const math::vec3& offset)        { obb_.set_offset(offset); }
    inline void update_bounding_boxes()                         { update_OBB(); update_AABB(); }

    inline void set_visibility(bool value)                      { visible_ = value; }
    inline bool is_visible() const                              { return visible_; }
    inline bool is_terrain() const                              { return is_terrain_; }

    inline void set_frustum_cull(bool value)                    { frustum_cull_ = value; }
    inline bool can_frustum_cull() const                        { return frustum_cull_; }
    inline void set_shadow_cull_face(uint32_t value)            { shadow_cull_face_ = value; }
    inline bool shadow_cull_face() const                        { return shadow_cull_face_; }

    inline void set_position(const math::vec3& newpos)          { trans_.set_position(newpos); }
    inline void set_orientation(const math::quat& newori)       { trans_.set_orientation(newori); }
    inline math::vec3 get_orientation_euler(bool deg=true)      { return trans_.get_orientation_euler(deg); }
    inline void set_scale(float scale)                          { trans_.set_scale(scale); }
    inline void rotate(float phi, float theta, float psi)       { trans_.rotate(phi, theta, psi); }
    inline void rotate(const math::vec3& axis, float angle)     { trans_.rotate(axis, angle); }
    inline void rotate(math::vec3&& axis, float angle)          { trans_.rotate(std::forward<math::vec3>(axis), angle); }
    inline void translate(const math::vec3& increment)          { trans_.translate(increment); }
    inline void translate(float x, float y, float z)            { trans_.translate(x, y, z); }
    inline void translate_x(float x)                            { trans_.translate_x(x); }
    inline void translate_y(float y)                            { trans_.translate_y(y); }
    inline void translate_z(float z)                            { trans_.translate_z(z); }

    // Reference
    inline void set_reference(hash_t hname) { reference_ = hname; has_reference_ = true; }
    inline void forget_reference()          { reference_ = 0; has_reference_ = false; }
    inline hash_t get_reference() const     { return reference_; }
    inline bool has_reference() const       { return has_reference_; }

#ifndef __DISABLE_EDITOR__
    // Set editor callback to clear selection on destruction
    inline void add_selection_reset_callback(Editor* editor, void (Editor::*selection_reset) (void))
    {
        editor_ = editor;
        selection_reset_ = selection_reset;
    }
    // Reset editor selection callback
    inline void remove_selection_reset_callback()
    {
        editor_ = nullptr;
        selection_reset_ = nullptr;
    }
#endif
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
