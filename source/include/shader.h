#ifndef SHADER_H
#define SHADER_H
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <GL/glew.h>

#include "math3d.h"
#include "wtypes.h"

namespace wcore
{

class Texture;
class Material;
class Light;

// Allows flexible shader resource query
struct ShaderResource
{
public:
    std::string vertex_shader;   // filename.ext for vertex shader
    std::string geometry_shader; // filename.ext for geometry shader
    std::string fragment_shader; // filename.ext for fragment shader

    std::vector<std::string> flags; // list of flags that will end up in #define directives

    //static const std::string SHADER_PATH;
    static constexpr hash_t VS = ".vert"_h; // extension for vertex shader
    static constexpr hash_t GS = ".geom"_h; // extension for geometry shader
    static constexpr hash_t FS = ".frag"_h; // extension for fragment shader

    ShaderResource(std::string&& resource_str,
                   std::string&& flags_str = "");
};

class Shader
{
private:
    GLuint ProgramID_;
    GLuint VertexShaderID_;
    GLuint GeometryShaderID_;
    GLuint FragmentShaderID_;

    std::map<hash_t, GLint> uniform_locations_;

    GLuint compile_shader(const std::string& shader_file,
                          GLenum ShaderType,
                          const std::vector<std::string>& flags);
    void parse_include(const std::string& line, std::string& shader_source);
    void parse_version(const std::string& line, std::string& shader_source);
    void setup_defines(std::string& shader_source, const std::vector<std::string>& flags);
    void link();
    void setup_uniform_map();
    void shader_error_report(GLuint ShaderID);
    void program_error_report();
    void program_active_report();
    void warn_uniform_unknown_type() const;

    std::vector<hash_t> defines_;
    static std::vector<std::string> global_defines_;

#ifdef __DEBUG__
    std::string name_;
    std::string glsl_version_;
    static uint32_t instance_count_;

    static void dbg_show_defines();
#endif

public:
    Shader() = delete;
    Shader(const ShaderResource& res);
    ~Shader();

    inline void use() const        { glUseProgram(ProgramID_); }
    inline void unuse() const      { glUseProgram(0); }
    inline GLuint get_program_id() { return ProgramID_; }
    inline bool is_variant(hash_t variant);

    template <typename T>
    bool send_uniform(hash_t name, const T& value) const
    {
        warn_uniform_unknown_type();
        return false;
    }
    template <typename T>
    bool send_uniform_array(hash_t name, T* array, int size) const
    {
        warn_uniform_unknown_type();
        return false;
    }
    template <typename T>
    bool send_uniforms(const T& value) const
    {
        warn_uniform_unknown_type();
        return false;
    }
    bool send_uniforms(const Light& light) const;
};

inline bool Shader::is_variant(hash_t variant)
{
    auto it = std::find(defines_.begin(), defines_.end(), variant);
    return (it!=defines_.end());
}


template <>
bool Shader::send_uniform<bool>(hash_t name, const bool& value) const;
template <>
bool Shader::send_uniform<float>(hash_t name, const float& value) const;
template <>
bool Shader::send_uniform<int>(hash_t name, const int& value) const;
/*template <>
bool Shader::send_uniform<uint32_t>(hash_t name, const uint32_t& value) const;*/
template <>
bool Shader::send_uniform<math::vec2>(hash_t name, const math::vec2& value) const;
template <>
bool Shader::send_uniform<math::vec3>(hash_t name, const math::vec3& value) const;
template <>
bool Shader::send_uniform<math::vec4>(hash_t name, const math::vec4& value) const;
template <>
bool Shader::send_uniform<math::mat2>(hash_t name, const math::mat2& value) const;
template <>
bool Shader::send_uniform<math::mat3>(hash_t name, const math::mat3& value) const;
template <>
bool Shader::send_uniform<math::mat4>(hash_t name, const math::mat4& value) const;
template <>
bool Shader::send_uniforms<Texture>(const Texture& texture) const;
template <>
bool Shader::send_uniforms<Material>(const Material& material) const;

template <>
bool Shader::send_uniform_array<float>(hash_t name, float* array, int size) const;

}

#endif // SHADER_H
