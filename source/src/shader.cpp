#include <cstring>
#include <set>
#include <filesystem>
#include <algorithm>

#include "shader.h"
#include "lights.h"
#include "material.h"
#include "texture.h"
#include "logger.h"
#include "material_common.h"
#include "file_system.h"
#include "error.h"

namespace fs = std::filesystem;

namespace wcore
{

std::vector<std::string> Shader::global_defines_ =
{
#ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
    "__EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__",
#endif
};

ShaderResource::ShaderResource(std::string&& resource_str,
                               std::string&& flags_str)
{
    // Tokenize resource string
    std::vector<std::string> resources;
    split_string(resource_str, resources, ';');

    // For each token, determine shader type and initialize corresponding field
    for(uint32_t ii=0; ii<resources.size(); ++ii)
    {
        fs::path file_name(resources[ii]);
        hash_t shader_type = H_(file_name.extension().string().c_str());

        switch(shader_type)
        {
            case VS:
                vertex_shader   = file_name;
                break;
            case GS:
                geometry_shader = file_name;
                break;
            case FS:
                fragment_shader = file_name;
                break;
            default:
                DLOGE("[Shader] Unknown / unsupported shader type: " + resources[ii], "shader");
        }
    }

    // Parse flags (for shader variants)
    if(flags_str.size())
        split_string(flags_str, flags, ';');
}

#ifdef __DEBUG__
uint32_t Shader::instance_count_ = 0;

void Shader::dbg_show_defines()
{
    if(global_defines_.size()==0) return;
    DLOGN("[Shader] Debug nonce.", "shader");
    DLOG("Global <i>#define</i>s in shaders:", "shader", Severity::LOW);
    for(const std::string& str: global_defines_)
    {
        DLOGI("<i>#define</i> <v>" + str + "</v>", "shader");
    }
}
#endif

Shader::Shader(const ShaderResource& res):
ProgramID_(0),
VertexShaderID_(0),
GeometryShaderID_(0),
FragmentShaderID_(0)
#ifdef __DEBUG__
, line_offset_(0)
#endif
{
#ifdef __DEBUG__
    if(++instance_count_ == 1) dbg_show_defines();
    // strip extension from vertex shader filename to get shader name, ugly but ok
    name_ = fs::path(res.vertex_shader).stem().string();
    DLOGS("[Shader] Creating new shader program:", "shader", Severity::LOW);
    DLOGI("name: <n>" + name_ + "</n>", "shader");

    for(int ii=0; ii<res.flags.size(); ++ii)
    {
        DLOGI("variant: <n>" + res.flags[ii] + "</n>", "shader");
    }
#endif
    // VERTEX SHADER
    if(!res.vertex_shader.empty())
    {
        VertexShaderID_   = compile_shader(res.vertex_shader, GL_VERTEX_SHADER, res.flags);
#ifdef __DEBUG__
        DLOGI("<g>Compiled</g> vertex shader from: <p>" + res.vertex_shader + "</p>", "shader");
#endif
    }

    // GEOMETRY SHADER
    if(!res.geometry_shader.empty())
    {
        GeometryShaderID_ = compile_shader(res.geometry_shader, GL_GEOMETRY_SHADER, res.flags);
#ifdef __DEBUG__
        DLOGI("<g>Compiled</g> geometry shader from: <p>" + res.geometry_shader + "</p>", "shader");
#endif
    }

    // FRAGMENT SHADER
    if(!res.fragment_shader.empty())
    {
        FragmentShaderID_ = compile_shader(res.fragment_shader, GL_FRAGMENT_SHADER, res.flags);
#ifdef __DEBUG__
        DLOGI("<g>Compiled</g> fragment shader from: <p>" + res.fragment_shader + "</p>", "shader");
#endif
    }

    // Link program
    link();
#ifdef __DEBUG__
    DLOGI("Shader program [" + std::to_string(ProgramID_) + "] linked.", "shader");
    program_active_report();
#endif

    // Register uniform locations
    setup_uniform_map();
    DLOGES("shader", Severity::LOW);
}


Shader::~Shader()
{
    #ifdef __DEBUG__
        DLOGN("[Shader] Destroying program <z>[" + std::to_string(ProgramID_) + "]</z> <n>" + name_ + "</n>", "shader");
    #endif
    glDetachShader(ProgramID_,VertexShaderID_);
    glDetachShader(ProgramID_,FragmentShaderID_);
    if(GeometryShaderID_)
    {
        glDetachShader(ProgramID_,GeometryShaderID_);
        glDeleteShader(GeometryShaderID_);
    }
    glDeleteShader(VertexShaderID_);
    glDeleteShader(FragmentShaderID_);
    glDeleteProgram(ProgramID_);
}

void Shader::program_active_report()
{
    #ifdef __DEBUG__
        // Display active attributes
        GLint active_attribs;
        glGetProgramiv(ProgramID_, GL_ACTIVE_ATTRIBUTES, &active_attribs);
        DLOGN("Detected " + std::to_string(active_attribs) + " active attributes:", "shader");

        for(GLint ii=0; ii<active_attribs; ++ii)
        {
            char name[33];
            GLsizei length;
            GLint   size;
            GLenum  type;

            glGetActiveAttrib(ProgramID_, ii, 32, &length, &size, &type, name);
            GLint loc = glGetAttribLocation(ProgramID_, name);

            DLOGI("<u>" + std::string(name) + "</u> [" + std::to_string(type) + "] loc=" + std::to_string(loc), "shader");
        }

        GLint active_unif;
        glGetProgramiv(ProgramID_, GL_ACTIVE_UNIFORMS, &active_unif);
        DLOGN("Detected " + std::to_string(active_unif) + " active uniforms:", "shader");

        for(GLint ii=0; ii<active_unif; ++ii)
        {
            GLchar name[33];
            GLsizei length=0;

            glGetActiveUniformName(ProgramID_, ii, 32, &length, name);
            GLint loc = glGetUniformLocation(ProgramID_, name);
            DLOGI("<u>" + std::string(name) + "</u> [" + std::to_string(loc) + "] ", "shader");
        }
    #endif // __DEBUG__
}

void Shader::setup_uniform_map()
{
    // Get number of active uniforms
    GLint num_active_uniforms;
    glGetProgramiv(ProgramID_, GL_ACTIVE_UNIFORMS, &num_active_uniforms);

    // For each uniform register name in map
    for(GLint ii=0; ii<num_active_uniforms; ++ii)
    {
        GLchar name[33];
        GLsizei length=0;

        glGetActiveUniformName(ProgramID_, ii, 32, &length, name);
        GLint location = glGetUniformLocation(ProgramID_, name);

        uniform_locations_.insert(std::make_pair(H_(name), location));
    }
}

void Shader::parse_include(const std::string& incline, std::string& shader_source)
{
    // Capture argument of include directive between quotes
    const char* incStr = "#include";
    uint32_t offset = sizeof(incStr) + 2;

    std::string file_name(incline.substr(offset, incline.length()-(offset+1)));
    std::string include_source(FILESYSTEM.get_file_as_string(file_name.c_str(), "root.folders.shaderinc"_h, "pack0"_h));
#ifdef __DEBUG__
    DLOGI("Dependency: <p>" + file_name + "</p>", "shader");
#endif

    // Append to source
    shader_source += include_source;
    // Update line offset
    line_offset_ += std::count(include_source.begin(), include_source.end(), '\n');
}

void Shader::parse_version(const std::string& line, std::string& shader_source)
{
    // Capture version string
    const char* verStr = "#version";
    if(line.substr(0,sizeof(verStr)).compare(verStr))
    {
#ifdef __DEBUG__
        DLOGW("Shader does not start with <i>#version</i> directive.", "shader");
#endif
        return;
    }

#ifdef __DEBUG__
    uint32_t offset = sizeof(verStr) + 1;
    glsl_version_ = line.substr(offset, line.length()-offset);
    DLOGI("<i>#version</i> <v>" + glsl_version_ + "</v>", "shader");
#endif

    shader_source += line + "\n";
}


void Shader::setup_defines(std::string& shader_source,
                           const std::vector<std::string>& flags)
{
    for(const std::string& str: global_defines_)
    {
        shader_source += "#define " + str + "\n";
    }
    for(const std::string& str: flags)
    {
        shader_source += "#define " + str + "\n";
        defines_.push_back(H_(str.c_str()));
    }
}

GLuint Shader::compile_shader(const std::string& shader_file,
                              GLenum ShaderType,
                              const std::vector<std::string>& flags)
{
    std::string shader_source_raw(FILESYSTEM.get_file_as_string(shader_file.c_str(), "root.folders.shader"_h, "pack0"_h));
    std::string shader_source;
    {
        std::stringstream ss(shader_source_raw);
        std::string line;

        // Parse version directive
        std::getline(ss, line);
        parse_version(line, shader_source);

        // First, add #define directives
        setup_defines(shader_source, flags);

        // Parse buffer
        while(std::getline(ss, line, '\n'))
        {
            if(!line.substr(0,8).compare("#include"))
                parse_include(line, shader_source);
            else
                shader_source += line + "\n";
        }
    }

    GLuint ShaderID = glCreateShader(ShaderType);
    if(ShaderID == 0)
    {
        DLOGF("Cannot create shader: <p>" + shader_file + "</p>", "shader");
        fatal("Cannot create shader: " + shader_file);
    }

    const GLchar* source = (const GLchar*) shader_source.c_str();
    glShaderSource(ShaderID, 1, &source, nullptr);
    glCompileShader(ShaderID);

    GLint isCompiled = 0;
    glGetShaderiv(ShaderID, GL_COMPILE_STATUS, &isCompiled);

    if(isCompiled == GL_FALSE)
    {
        shader_error_report(ShaderID);

        //We don't need the shader anymore.
        glDeleteShader(ShaderID);
        DLOGF("Shader will not compile: <p>" + shader_file + "</p>", "shader");
        DLOGI("Line offset is: <h>" + std::to_string(line_offset_) + "</h>", "shader");
        fatal("Shader will not compile: " + shader_file);
    }

    return ShaderID;
}

void Shader::link()
{
    ProgramID_ = glCreateProgram();
    glAttachShader(ProgramID_, VertexShaderID_);
    if(GeometryShaderID_) glAttachShader(ProgramID_, GeometryShaderID_);
    glAttachShader(ProgramID_, FragmentShaderID_);
    glLinkProgram(ProgramID_);

    GLint isLinked = 0;
    glGetProgramiv(ProgramID_, GL_LINK_STATUS, (int*) &isLinked);

    if(isLinked == GL_FALSE)
    {
        program_error_report();
        DLOGF("Unable to link shaders.", "shader");
        DLOGI("Line offset is: <h>" + std::to_string(line_offset_) + "</h>", "shader");

        //We don't need the program anymore.
        glDeleteProgram(ProgramID_);
        //Don't leak shaders either.
        glDeleteShader(VertexShaderID_);
        glDeleteShader(FragmentShaderID_);

        fatal("Unable to link shaders.");
    }
}

void Shader::shader_error_report(GLuint ShaderID)
{
    char* log = nullptr;
    GLsizei logsize = 0;

    glGetShaderiv(ShaderID, GL_INFO_LOG_LENGTH, &logsize);

    log = (char*) malloc(logsize + 1);
    if(log == nullptr)
    {
        DLOGF("Cannot allocate memory for Shader Error Report.", "shader");
        fatal("Cannot allocate memory for Shader Error Report.");
    }

    memset(log, '\0', logsize + 1);
    glGetShaderInfoLog(ShaderID, logsize, &logsize, log);
    fprintf(stderr, "%s\n", log);
    free(log);
}

void Shader::program_error_report()
{
    char* log = nullptr;
    GLsizei logsize = 0;

    glGetProgramiv(ProgramID_, GL_INFO_LOG_LENGTH, &logsize);

    log = (char*) malloc(logsize + 1);
    if(log == nullptr)
    {
        DLOGF("Cannot allocate memory for Program Error Report.", "shader");
        fatal("Cannot allocate memory for Program Error Report.");
    }

    memset(log, '\0', logsize + 1);
    glGetProgramInfoLog(ProgramID_, logsize, &logsize, log);
    fprintf(stderr, "%s\n", log);
    free(log);
}

#ifdef __DEBUG__
static std::set<hash_t> marked; // So that we don't warn twice for the same uniform
static inline void warn_unknown_uniform(const std::string& shaderName, hash_t name)
{
    hash_t shname = H_(shaderName.c_str());
    hash_t id = shname ^ name;

    if(marked.find(id) == marked.end())
    {
        std::stringstream ss;
        ss << "[Shader] [<n>" << shaderName << "</n>] Unknown uniform name: " << HRESOLVE(name) << " <u>(" << name << ")</u>";
        DLOGW(ss.str(), "shader");
        marked.insert(id);
    }
}
#endif

void Shader::warn_uniform_unknown_type() const
{
#ifdef __DEBUG__
    DLOGW("[Shader] Unknown type in send_uniform()/send_uniforms().", "shader");
#endif
}

template <>
bool Shader::send_uniform<bool>(hash_t name, const bool& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
#ifdef __DEBUG__
        warn_unknown_uniform(name_, name);
#endif
        return false;
    }

    glUniform1i(it->second, value);
    return true;
}

template <>
bool Shader::send_uniform<float>(hash_t name, const float& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
#ifdef __DEBUG__
        warn_unknown_uniform(name_, name);
#endif
        return false;
    }

    glUniform1f(it->second, value);
    return true;
}

template <>
bool Shader::send_uniform<int>(hash_t name, const int& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
#ifdef __DEBUG__
        warn_unknown_uniform(name_, name);
#endif
        return false;
    }

    glUniform1i(it->second, value);
    return true;
}
/*
template <>
bool Shader::send_uniform<uint32_t>(hash_t name, const uint32_t& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
#ifdef __DEBUG__
        warn_unknown_uniform(name_, name);
#endif
        return false;
    }

    glUniform1ui(location, value);
    return true;
}*/

template <>
bool Shader::send_uniform<math::vec2>(hash_t name, const math::vec2& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
#ifdef __DEBUG__
        warn_unknown_uniform(name_, name);
#endif
        return false;
    }

    glUniform2fv(it->second, 1, (const GLfloat*)&value);
    return true;
}

template <>
bool Shader::send_uniform<math::vec3>(hash_t name, const math::vec3& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
#ifdef __DEBUG__
        warn_unknown_uniform(name_, name);
#endif
        return false;
    }

    glUniform3fv(it->second, 1, (const GLfloat*)&value);
    return true;
}

template <>
bool Shader::send_uniform<math::vec4>(hash_t name, const math::vec4& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
#ifdef __DEBUG__
        warn_unknown_uniform(name_, name);
#endif
        return false;
    }

    glUniform4fv(it->second, 1, (const GLfloat*)&value);
    return true;
}

template <>
bool Shader::send_uniform<math::mat2>(hash_t name, const math::mat2& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
#ifdef __DEBUG__
        warn_unknown_uniform(name_, name);
#endif
        return false;
    }

    glUniformMatrix2fv(it->second, 1, GL_FALSE, value.get_pointer());
    return true;
}

template <>
bool Shader::send_uniform<math::mat3>(hash_t name, const math::mat3& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
#ifdef __DEBUG__
        warn_unknown_uniform(name_, name);
#endif
        return false;
    }

    glUniformMatrix3fv(it->second, 1, GL_FALSE, value.get_pointer());
    return true;
}

template <>
bool Shader::send_uniform<math::mat4>(hash_t name, const math::mat4& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
#ifdef __DEBUG__
        warn_unknown_uniform(name_, name);
#endif
        return false;
    }

    glUniformMatrix4fv(it->second, 1, GL_FALSE, value.get_pointer());
    return true;
}

template <>
bool Shader::send_uniforms<Texture>(const Texture& texture) const
{
    for (GLuint ii = 0; ii < texture.get_num_textures(); ++ii)
    {
        send_uniform<int>(texture.get_sampler_name(ii), ii);
    }
    return true;
}

template <>
bool Shader::send_uniforms<Material>(const Material& material) const
{
    bool has_albedo    = material.has_texture(TextureUnit::ALBEDO);
    bool has_normal    = material.has_texture(TextureUnit::NORMAL);
    bool has_depth     = material.has_texture(TextureUnit::DEPTH);
    bool has_metallic  = material.has_texture(TextureUnit::METALLIC);
    bool has_ao        = material.has_texture(TextureUnit::AO);
    bool has_roughness = material.has_texture(TextureUnit::ROUGHNESS);

    // Block 0 -> Albedo only
    send_uniform("mt.b_has_albedo"_h, has_albedo);
    send_uniform("mt.b_use_normal_map"_h, has_normal);
    send_uniform("mt.b_use_parallax_map"_h, has_depth);
    send_uniform("mt.b_has_metallic"_h, has_metallic);
    send_uniform("mt.b_has_ao"_h, has_ao);
    send_uniform("mt.b_has_roughness"_h, has_roughness);

    if(has_albedo)
    {
        send_uniform<int>(material.get_texture().unit_to_sampler_name(TextureUnit::BLOCK0),
                          material.get_texture().get_unit_index(TextureUnit::BLOCK0));
    }
    else
        send_uniform("mt.v3_albedo"_h, material.get_albedo());

    // Block 1 -> Normal & Depth
    if(has_normal || has_depth)
    {
        send_uniform<int>(material.get_texture().unit_to_sampler_name(TextureUnit::BLOCK1),
                          material.get_texture().get_unit_index(TextureUnit::BLOCK1));
    }
    if(has_depth)
        send_uniform("mt.f_parallax_height_scale"_h, material.get_parallax_height_scale());

    // Block 2 -> Metallic, AO & Roughness
    if(has_metallic || has_ao || has_roughness)
    {
        send_uniform<int>(material.get_texture().unit_to_sampler_name(TextureUnit::BLOCK2),
                          material.get_texture().get_unit_index(TextureUnit::BLOCK2));
    }
    if(!has_metallic)
        send_uniform("mt.f_metallic"_h, material.get_metallic());
    if(!has_roughness)
        send_uniform("mt.f_roughness"_h, material.get_roughness());

/*
    if((has_map = material.has_texture(TextureUnit::ALBEDO)))
    {
        send_uniform<int>(material.get_texture().unit_to_sampler_name(TextureUnit::ALBEDO),
                          material.get_texture().get_unit_index(TextureUnit::ALBEDO));
    }
    else
    {
        send_uniform("mt.v3_albedo"_h, material.get_albedo());
    }
    send_uniform("mt.b_has_albedo"_h, has_map);

    if((has_map = material.has_texture(TextureUnit::AO)))
    {
        send_uniform<int>(material.get_texture().unit_to_sampler_name(TextureUnit::AO),
                          material.get_texture().get_unit_index(TextureUnit::AO));
    }
    send_uniform("mt.b_has_ao"_h, has_map);

    if((has_map = material.has_texture(TextureUnit::METALLIC)))
    {
        send_uniform<int>(material.get_texture().unit_to_sampler_name(TextureUnit::METALLIC),
                          material.get_texture().get_unit_index(TextureUnit::METALLIC));
    }
    else
    {
        send_uniform("mt.f_metallic"_h, material.get_metallic());
    }
    send_uniform("mt.b_has_metallic"_h, has_map);

    if((has_map = material.has_texture(TextureUnit::ROUGHNESS)))
    {
        send_uniform<int>(material.get_texture().unit_to_sampler_name(TextureUnit::ROUGHNESS),
                          material.get_texture().get_unit_index(TextureUnit::ROUGHNESS));
    }
    else
    {
        send_uniform("mt.f_roughness"_h, material.get_roughness());
    }
    send_uniform("mt.b_has_roughness"_h, has_map);

    if((has_map = material.has_texture(TextureUnit::NORMAL)))
    {
        send_uniform<int>(material.get_texture().unit_to_sampler_name(TextureUnit::NORMAL),
                          material.get_texture().get_unit_index(TextureUnit::NORMAL));
    }
    send_uniform("mt.b_use_normal_map"_h, has_map);

    if((has_map = material.has_texture(TextureUnit::DEPTH)))
    {
        send_uniform<int>(material.get_texture().unit_to_sampler_name(TextureUnit::DEPTH),
                          material.get_texture().get_unit_index(TextureUnit::DEPTH));
        send_uniform("mt.f_parallax_height_scale"_h, material.get_parallax_height_scale());
    }
    send_uniform("mt.b_use_parallax_map"_h, has_map);
*/
    return true;
}

bool Shader::send_uniforms(const Light& light) const
{
    // Each child Light class has its own layout
    // Let polymorphism take care of this
    light.update_uniforms(*this);
    return true;
}

template<>
bool Shader::send_uniform_array<float>(hash_t name, float* array, int size) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
#ifdef __DEBUG__
        warn_unknown_uniform(name_, name);
#endif
        return false;
    }

    glUniform1fv(it->second, size, array);
    return true;
}


}
