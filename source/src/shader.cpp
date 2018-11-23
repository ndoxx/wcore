#include <stdexcept>
#include <cstring>

#include "shader.h"
#include "lights.h"
#include "material.h"
#include "texture.h"
#include "logger.h"
#include "material_common.h"

namespace wcore
{

std::vector<std::string> Shader::global_defines_ =
{
#ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
    "__EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__",
#endif
};

const std::string ShaderResource::SHADER_PATH("../res/shaders/");

ShaderResource::ShaderResource(std::string&& resource_str,
                               std::string&& flags_str)
{
    // Tokenize resource string
    std::vector<std::string> resources;
    split_string(resource_str, resources, ';');

    // For each token, determine shader type and initialize corresponding field
    for(uint32_t ii=0; ii<resources.size(); ++ii)
    {
        std::vector<std::string> name_extension;
        split_string(resources[ii], name_extension, '.');

        hashstr_t shader_type = HS_(name_extension[1].c_str());

        switch(shader_type)
        {
            case VS:
                vertex_shader   = SHADER_PATH + resources[ii];
                break;
            case GS:
                geometry_shader = SHADER_PATH + resources[ii];
                break;
            case FS:
                fragment_shader = SHADER_PATH + resources[ii];
                break;
            default:
                std::cout << "ERROR" << std::endl;
        }

    }

    // Parse flags (for shader variants)
    if(flags_str.size())
        split_string(flags_str, flags, ';');
}

#ifdef __DEBUG_SHADER__
uint32_t Shader::instance_count_ = 0;

void Shader::dbg_show_defines()
{
    if(global_defines_.size()==0) return;
    DLOGN("[Shader] Debug nonce.");
    DLOG("Global <i>#define</i>s in shaders:");
    for(const std::string& str: global_defines_)
    {
        DLOGI("<i>#define</i> <v>" + str + "</v>");
    }
}
#endif

Shader::Shader(const ShaderResource& res):
ProgramID_(0),
VertexShaderID_(0),
GeometryShaderID_(0),
FragmentShaderID_(0)
{
#ifdef __DEBUG_SHADER__
    if(++instance_count_ == 1) dbg_show_defines();
    auto const pos_dot = res.vertex_shader.find_last_of('.');
    auto const pos_slh = res.vertex_shader.find_last_of('/');
    name_ = res.vertex_shader.substr(pos_slh+1, pos_dot-pos_slh-1);
    DLOGS("[Shader] Creating new shader program: <n>" + name_ + "</n>");
#endif
    // VERTEX SHADER
    if(res.vertex_shader.size())
    {
        VertexShaderID_   = compile_shader(res.vertex_shader, GL_VERTEX_SHADER, res.flags);
#ifdef __DEBUG_SHADER__
        DLOGI("<g>Compiled</g> vertex shader from: <p>" + res.vertex_shader + "</p>");
#endif
    }

    // GEOMETRY SHADER
    if(res.geometry_shader.size())
    {
        GeometryShaderID_ = compile_shader(res.geometry_shader, GL_GEOMETRY_SHADER, res.flags);
#ifdef __DEBUG_SHADER__
        DLOGI("<g>Compiled</g> geometry shader from: <p>" + res.geometry_shader + "</p>");
#endif
    }

    // FRAGMENT SHADER
    if(res.fragment_shader.size())
    {
        FragmentShaderID_ = compile_shader(res.fragment_shader, GL_FRAGMENT_SHADER, res.flags);
#ifdef __DEBUG_SHADER__
        DLOGI("<g>Compiled</g> fragment shader from: <p>" + res.fragment_shader + "</p>");
#endif
    }

    // Link program
    link();
#ifdef __DEBUG_SHADER__
    DLOGI("Shader program [" + std::to_string(ProgramID_) + "] linked.");
    program_active_report();
#endif

    // Register uniform locations
    setup_uniform_map();
}


Shader::~Shader()
{
    #ifdef __DEBUG_SHADER_VERBOSE__
        DLOGN("[Shader] Destroying program <z>[" + std::to_string(ProgramID_) + "]</z> <n>" + name_ + "</n>");
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
    #ifdef __DEBUG_SHADER_VERBOSE__
        // Display active attributes
        GLint active_attribs;
        glGetProgramiv(ProgramID_, GL_ACTIVE_ATTRIBUTES, &active_attribs);
        DLOGN("Detected " + std::to_string(active_attribs) + " active attributes:");

        for(GLint ii=0; ii<active_attribs; ++ii)
        {
            char name[33];
            GLsizei length;
            GLint   size;
            GLenum  type;

            glGetActiveAttrib(ProgramID_, ii, 32, &length, &size, &type, name);
            GLint loc = glGetAttribLocation(ProgramID_, name);

            DLOGI("<u>" + std::string(name) + "</u> [" + std::to_string(type) + "] loc=" + std::to_string(loc));
        }

        GLint active_unif;
        glGetProgramiv(ProgramID_, GL_ACTIVE_UNIFORMS, &active_unif);
        DLOGN("Detected " + std::to_string(active_unif) + " active uniforms:");

        for(GLint ii=0; ii<active_unif; ++ii)
        {
            GLchar name[33];
            GLsizei length=0;

            glGetActiveUniformName(ProgramID_, ii, 32, &length, name);
            GLint loc = glGetUniformLocation(ProgramID_, name);

            DLOGI("<u>" + std::string(name) + "</u> [" + std::to_string(ii) + "] loc=" + std::to_string(loc));
        }
    #endif // __DEBUG_SHADER_VERBOSE__
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
    std::string file_path("../res/shaders/include/");
    file_path += incline.substr(offset, incline.length()-(offset+1));

    // Open included file
#ifdef __DEBUG_SHADER_VERBOSE__
    DLOGI("Dependency: <p>" + file_path + "</p>");
#endif
    std::ifstream include_file(file_path);

    if(!include_file.is_open())
    {
#ifdef __DEBUG_SHADER_VERBOSE__
        DLOGW("Unable to open file, skipping.");
#endif
        return;
    }

    // Read file to buffer
    std::string include_source((std::istreambuf_iterator<char>(include_file)),
                               std::istreambuf_iterator<char>());

    // Append to source
    shader_source += include_source;
}

void Shader::parse_version(const std::string& line, std::string& shader_source)
{
    // Capture version string
    const char* verStr = "#version";
    if(line.substr(0,sizeof(verStr)).compare(verStr))
    {
#ifdef __DEBUG_SHADER__
        DLOGW("Shader does not start with <i>#version</i> directive.");
#endif
        return;
    }

#ifdef __DEBUG_SHADER__
    uint32_t offset = sizeof(verStr) + 1;
    glsl_version_ = line.substr(offset, line.length()-offset);
    DLOGI("<i>#version</i> <v>" + glsl_version_ + "</v>");
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
    }
}

GLuint Shader::compile_shader(const std::string& ShaderPath,
                              GLenum ShaderType,
                              const std::vector<std::string>& flags)
{
    std::string ShaderSource;
    {
        std::ifstream file(ShaderPath);
        if(!file.is_open())
        {
            DLOGF("File <p>" + ShaderPath + "</p> not found.");
            throw std::runtime_error("File " + ShaderPath + " not found.");

        }

        // Parse version directive
        std::string line;
        getline(file, line);
        parse_version(line, ShaderSource);

        // First, add #define directives
        setup_defines(ShaderSource, flags);

        // Parse file
        while(getline(file,line))
        {
            if(!line.substr(0,8).compare("#include"))
                parse_include(line, ShaderSource);
            else
                ShaderSource += line + "\n";
        }
        file.close();
    }

    GLuint ShaderID = glCreateShader(ShaderType);
    if(ShaderID == 0)
    {
        DLOGF("Cannot create shader: <p>" + ShaderPath + "</p>");
        throw std::runtime_error("Cannot create shader: " + ShaderPath);
    }

    const GLchar* source = (const GLchar*) ShaderSource.c_str();
    glShaderSource(ShaderID, 1, &source, nullptr);
    glCompileShader(ShaderID);

    GLint isCompiled = 0;
    glGetShaderiv(ShaderID, GL_COMPILE_STATUS, &isCompiled);

    if(isCompiled == GL_FALSE)
    {
        shader_error_report(ShaderID);

        //We don't need the shader anymore.
        glDeleteShader(ShaderID);
        DLOGF("Shader will not compile: <p>" + ShaderPath + "</p>");
        throw std::runtime_error("Shader will not compile: " + ShaderPath);
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
        DLOGF("Unable to link shaders.");

        //We don't need the program anymore.
        glDeleteProgram(ProgramID_);
        //Don't leak shaders either.
        glDeleteShader(VertexShaderID_);
        glDeleteShader(FragmentShaderID_);

        throw std::runtime_error("Unable to link shaders.");
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
        DLOGF("Cannot allocate memory for Shader Error Report.");
        throw std::runtime_error("Cannot allocate memory for Shader Error Report.");
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
        DLOGF("Cannot allocate memory for Program Error Report.");
        throw std::runtime_error("Cannot allocate memory for Program Error Report.");
    }

    memset(log, '\0', logsize + 1);
    glGetProgramInfoLog(ProgramID_, logsize, &logsize, log);
    fprintf(stderr, "%s\n", log);
    free(log);
}

#ifdef __DEBUG_SHADER__
static inline void warn_unknown_uniform(const std::string& shaderName, hash_t name)
{
    std::stringstream ss;
    ss << "[Shader] [<n>" << shaderName << "</n>] Unknown uniform name: <u>" << name << "</u>";
    DLOGW(ss.str());
}
#endif

void Shader::warn_uniform_unknown_type() const
{
#ifdef __DEBUG__
    DLOGW("[Shader] Unknown type in send_uniform()/send_uniforms().");
#endif
}

template <>
bool Shader::send_uniform<bool>(hash_t name, const bool& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
#ifdef __DEBUG_SHADER__
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
#ifdef __DEBUG_SHADER__
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
#ifdef __DEBUG_SHADER__
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
#ifdef __DEBUG_SHADER__
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
#ifdef __DEBUG_SHADER__
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
#ifdef __DEBUG_SHADER__
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
#ifdef __DEBUG_SHADER__
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
#ifdef __DEBUG_SHADER__
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
#ifdef __DEBUG_SHADER__
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
#ifdef __DEBUG_SHADER__
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
    bool has_map = false;

    if((has_map = material.has_texture(TextureUnit::ALBEDO)))
    {
        send_uniform<int>(Texture::unit_to_sampler_name(TextureUnit::ALBEDO),
                          material.get_texture().get_unit_index(TextureUnit::ALBEDO));
    }
    else
    {
        send_uniform(H_("mt.v3_albedo"), material.get_albedo());
    }
    send_uniform(H_("mt.b_has_albedo"), has_map);

    if((has_map = material.has_texture(TextureUnit::AO)))
    {
        send_uniform<int>(Texture::unit_to_sampler_name(TextureUnit::AO),
                          material.get_texture().get_unit_index(TextureUnit::AO));
    }
    send_uniform(H_("mt.b_has_ao"), has_map);

    if((has_map = material.has_texture(TextureUnit::METALLIC)))
    {
        send_uniform<int>(Texture::unit_to_sampler_name(TextureUnit::METALLIC),
                          material.get_texture().get_unit_index(TextureUnit::METALLIC));
    }
    else
    {
        send_uniform(H_("mt.f_metallic"), material.get_metallic());
    }
    send_uniform(H_("mt.b_has_metallic"), has_map);

    if((has_map = material.has_texture(TextureUnit::ROUGHNESS)))
    {
        send_uniform<int>(Texture::unit_to_sampler_name(TextureUnit::ROUGHNESS),
                          material.get_texture().get_unit_index(TextureUnit::ROUGHNESS));
    }
    else
    {
        send_uniform(H_("mt.f_roughness"), material.get_roughness());
    }
    send_uniform(H_("mt.b_has_roughness"), has_map);

    if(material.has_normal_map())
    {
        send_uniform<int>(Texture::unit_to_sampler_name(TextureUnit::NORMAL),
                          material.get_texture().get_unit_index(TextureUnit::NORMAL));
    }
    send_uniform(H_("mt.b_use_normal_map"), material.has_normal_map());

    if(material.has_parallax_map())
    {
        send_uniform<int>(Texture::unit_to_sampler_name(TextureUnit::DEPTH),
                          material.get_texture().get_unit_index(TextureUnit::DEPTH));
        send_uniform(H_("mt.f_parallax_height_scale"), material.get_parallax_height_scale());
    }
    send_uniform(H_("mt.b_use_parallax_map"), material.has_parallax_map());

    return true;
}

bool Shader::send_uniforms(std::shared_ptr<const Light> plight) const
{
    // Each child Light class has its own layout
    // Let polymorphism take care of this
    plight->update_uniforms(*this);
    return true;
}

}
