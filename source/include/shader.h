#ifndef SHADER_H
#define SHADER_H
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
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
public:
    Shader() = delete;
    Shader(const ShaderResource& res);
    ~Shader();

    // Activate program
    inline void use() const        { glUseProgram(ProgramID_); }
    // Disable program
    inline void unuse() const      { glUseProgram(0); }
    // Get OpenGL program ID
    inline GLuint get_program_id() { return ProgramID_; }
    // Is this program a specified variant
    inline bool is_variant(hash_t variant);

    // Uniform management
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

    // Reload program from source, swap program only if compilation was successful
    bool reload();

#ifdef __DEBUG__
    // Reload all hotswappable programs
    static void dbg_hotswap();
#endif

private:
    // Compile a shader from file name, specifying shader type and setup defines if any
    GLuint compile_shader(const std::string& shader_file,
                          GLenum ShaderType,
                          const std::vector<std::string>& flags);
    // Replace an #include directive by actual code from the file it points to
    void parse_include(const std::string& line, std::string& shader_source);
    // Parse a #pragma directive
    void parse_pragma(const std::string& line, std::string& shader_source);
    // Parse openGL version from #version directive in source
    void parse_version(const std::string& line, std::string& shader_source);
    // Write a #define directive for each flag in flags
    void setup_defines(std::string& shader_source, const std::vector<std::string>& flags);
    // Link program
    bool link();
    // Associate each active uniform to its uniform hname engine-side
    void setup_uniform_map();
    // Print the error report generated when shader compilation failed, populate a set of error line numbers
    void shader_error_report(GLuint ShaderID, std::set<int>& errlines);
    // Print the error report generated on program linking failure
    void program_error_report();
    // Print the list of active attributes and uniforms detected after a successful compilation/linking
    void program_active_report();
    // Display a warning message for when a uniform of unknown type is sent via send_uniform_x()
    void warn_uniform_unknown_type() const;

private:
    ShaderResource resource_;
    GLuint ProgramID_;
    GLuint VertexShaderID_;
    GLuint GeometryShaderID_;
    GLuint FragmentShaderID_;

    std::map<hash_t, GLint> uniform_locations_; // [uniform hname, location]

    std::vector<hash_t> defines_; // list of all the defines specified
    static std::vector<std::string> global_defines_; // list of all the global defines

#ifdef __DEBUG__
    std::string name_;
    std::string glsl_version_;

    uint32_t instance_index_;
    static uint32_t instance_count_;
    static std::map<uint32_t,Shader*> hotswap_shaders_; // Map of all shaders that enable hot swapping

    // Displays the defines specified by this instance
    static void dbg_show_defines();
#endif
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
