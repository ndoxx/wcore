#include <cassert>
#include <vector>
#include <sstream>
#include <GL/glew.h>


#include "texture.h"
#include "pixel_buffer.h"
#include "shader.h"
#include "logger.h"
#include "algorithms.h"
#include "config.h"
#include "file_system.h"
#include "error.h"
#include "png_loader.h"

namespace wcore
{

static std::vector<std::map<TextureUnit, hash_t>> SAMPLER_NAMES =
{
    // Sampler names for sampler group 1
    {
        {TextureUnit::BLOCK0, "mt.sg1.block0Tex"_h},
        {TextureUnit::BLOCK1, "mt.sg1.block1Tex"_h},
        {TextureUnit::BLOCK2, "mt.sg1.block2Tex"_h}
    },

    // Sampler names for sampler group 2
    {
        {TextureUnit::BLOCK0, "mt.sg2.block0Tex"_h},
        {TextureUnit::BLOCK1, "mt.sg2.block1Tex"_h},
        {TextureUnit::BLOCK2, "mt.sg2.block2Tex"_h}
    }
};

static const uint32_t SAMPLER_GROUP_SIZE = 3;

static std::map<GLenum, GLenum> DATA_TYPES =
{
    {GL_SRGB_ALPHA,        GL_FLOAT},
    {GL_RGB16F,            GL_FLOAT},
    {GL_RGBA16F,           GL_FLOAT},
    {GL_RG16_SNORM,        GL_UNSIGNED_BYTE},
    {GL_RGB16_SNORM,       GL_UNSIGNED_BYTE},
    {GL_DEPTH32F_STENCIL8, GL_FLOAT_32_UNSIGNED_INT_24_8_REV},
    {GL_DEPTH24_STENCIL8,  GL_UNSIGNED_INT_24_8},
    {GL_RGB8,              GL_BYTE},
    {GL_R8,                GL_BYTE},
};

static PngLoader PNG_LOADER;

static bool handle_filter(TextureFilter filter, GLenum target)
{
    bool has_mipmap = (filter & TextureFilter::MIN_NEAREST_MIPMAP_NEAREST)
                   || (filter & TextureFilter::MIN_LINEAR_MIPMAP_NEAREST)
                   || (filter & TextureFilter::MIN_NEAREST_MIPMAP_LINEAR)
                   || (filter & TextureFilter::MIN_LINEAR_MIPMAP_LINEAR);

    // Magnification filter
    glTexParameterf(target, GL_TEXTURE_MAG_FILTER, bool(filter & MAG_LINEAR) ? GL_LINEAR : GL_NEAREST);

    // Minification filter
    uint16_t minfilter = filter & ~(1 << 0); // Clear mag filter bit

    switch(minfilter)
    {
        case MIN_NEAREST:
        {
            glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            break;
        }
        case MIN_LINEAR:
        {
            glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            break;
        }
        case MIN_NEAREST_MIPMAP_NEAREST:
        {
            glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            break;
        }
        case MIN_LINEAR_MIPMAP_NEAREST:
        {
            glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            break;
        }
        case MIN_NEAREST_MIPMAP_LINEAR:
        {
            glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
            break;
        }
        case MIN_LINEAR_MIPMAP_LINEAR:
        {
            glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            break;
        }
    }

    return has_mipmap;
}

static void handle_addressUV(TextureWrap wrap_param, GLenum target)
{
    switch(wrap_param)
    {
        case TextureWrap::REPEAT:
        {
            glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
            break;
        }
        case TextureWrap::MIRRORED_REPEAT:
        {
            glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
            glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
            break;
        }
        case TextureWrap::CLAMP_TO_EDGE:
        {
            glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            break;
        }
    }
}

static GLenum internal_format_to_data_type(GLenum iformat)
{
    auto it = DATA_TYPES.find(iformat);
    if(it!=DATA_TYPES.end())
        return it->second;
    return GL_UNSIGNED_BYTE;
}

static GLenum fix_internal_format(GLenum iformat, hash_t sampler_name)
{
    // Load Albedo / Diffuse textures as sRGB to avoid double gamma-correction.
    if(sampler_name == "mt.sg1.block0Tex"_h || sampler_name == "mt.sg2.block0Tex"_h)
        return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
    // Do not use DXT1 compression on normal-depth texure (block1) because it will screw up the normals
    else if(sampler_name == "mt.sg1.block1Tex"_h || sampler_name == "mt.sg2.block1Tex"_h)
        return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;//GL_RGBA;

    return iformat;
}

Texture::Texture(const TextureDescriptor& descriptor)
{
#ifdef __DEBUG__
    std::stringstream ss;
    if(descriptor.is_wat)
        ss << "[Texture] New texture from <h>Watfile</h>: <n>" << HRESOLVE(descriptor.resource_id) << "</n>";
    else
        ss << "[Texture] New texture from <h>XML</h>: <n>" << HRESOLVE(descriptor.resource_id) << "</n>";
    DLOGN(ss.str(), "texture");
#endif

    // Texture data blocks
    std::vector<unsigned char*> data_ptrs =
    {
        descriptor.block0_data,
        descriptor.block1_data,
        descriptor.block2_data
    };

    sampler_group_ = descriptor.sampler_group;
    unit_flags_    = descriptor.unit_flags;

    // Containers to be submitted to OpenGL
    std::vector<TextureFilter> filters;
    std::vector<GLenum> formats;
    std::vector<GLenum> internalFormats;
    std::vector<unsigned char*> data;
    uint32_t width  = 0;
    uint32_t height = 0;

    uint32_t ii=0;
    for(auto&& [key, sampler_name]: SAMPLER_NAMES[sampler_group_-1])
    {
        if(descriptor.has_unit(key))
        {
            // Register a sampler name for each unit
            unit_indices_[key] = uniform_sampler_names_.size() + (sampler_group_-1)*SAMPLER_GROUP_SIZE;
            uniform_sampler_names_.push_back(sampler_name);

            data.push_back(data_ptrs[ii]);
            filters.push_back(descriptor.parameters.filter);
            formats.push_back(descriptor.parameters.format);
            internalFormats.push_back(fix_internal_format(descriptor.parameters.internal_format, sampler_name));
            width = descriptor.width;
            height = descriptor.height;
        }
        ++ii;
    }

    // Send to OpenGL
    n_units_ = data.size();
    generate_texture_units(n_units_,
                           width,
                           height,
                           &data[0],
                           &internalFormats[0],
                           &formats[0],
                           filters,
                           descriptor.parameters.wrap,
                           false);
}

Texture::Texture(std::istream& stream):
unit_flags_(0),
sampler_group_(1)
{
    // Sanity check on stream
    if(!stream.good())
    {
        DLOGE("[Texture] Failed to create texture from stream: stream is bad.", "texture");
        return;
    }

    // ASSUME stream to png file
    PixelBuffer* px_buf = PNG_LOADER.load_png(stream);

#if __DEBUG__
    DLOGN("[PixelBuffer]", "texture");
    if(dbg::LOG.get_channel_verbosity("texture"_h) == 3u)
        px_buf->debug_display();
#endif

    TextureFilter filter = TextureFilter(TextureFilter::MAG_LINEAR | TextureFilter::MIN_LINEAR);
    GLenum internal_format = GL_RGB;
    GLenum format = GL_RGB;
    unsigned char* data = px_buf->get_data_pointer();
    generate_texture_units(1,
                           px_buf->get_width(),
                           px_buf->get_height(),
                           &data,
                           &internal_format,
                           &format,
                           std::vector<TextureFilter>{filter},
                           TextureWrap::CLAMP_TO_EDGE,
                           false);

    delete px_buf;
}

Texture::Texture(const std::vector<hash_t>& sampler_names,
        const std::vector<TextureFilter>& filters,
        const std::vector<uint32_t>& internalFormats,
        const std::vector<uint32_t>& formats,
        uint32_t width,
        uint32_t height,
        TextureWrap wrap_param,
        bool lazy_mipmap):
unit_flags_(0),
sampler_group_(1),
uniform_sampler_names_(sampler_names)
{
    generate_texture_units(sampler_names.size(),
                           width,
                           height,
                           nullptr,
                           (GLenum*)&internalFormats[0],
                           (GLenum*)&formats[0],
                           filters,
                           wrap_param,
                           lazy_mipmap);
}

Texture::~Texture()
{
    if(texture_ids_)
    {
        glDeleteTextures(n_units_, texture_ids_);
        delete[] texture_ids_;
    }
}

void Texture::bind_sampler(const Shader& shader, TextureUnit unit) const
{
    shader.send_uniform<int>(SAMPLER_NAMES[sampler_group_-1].at(unit), unit_indices_.at(unit));
}

void Texture::bind_all() const
{
    for(uint32_t ii=0; ii<n_units_; ++ii)
        bind(ii,ii);
}

void Texture::bind(uint32_t unit, uint32_t index) const
{
    glActiveTexture(GL_TEXTURE0 + unit + (sampler_group_-1)*SAMPLER_GROUP_SIZE);
    glBindTexture(GL_TEXTURE_2D, texture_ids_[index]);
}

void Texture::unbind() const
{
    for(uint32_t ii=0; ii<n_units_; ++ii)
    {
        glActiveTexture(GL_TEXTURE0 + ii);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Texture::generate_mipmaps(uint32_t index,
                               uint32_t base_level,
                               uint32_t max_level) const
{
    glBindTexture(GL_TEXTURE_2D, texture_ids_[index]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, base_level);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, max_level);
    glGenerateMipmap(GL_TEXTURE_2D);
    GLfloat maxAnisotropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
    glTexParameterf(GL_TEXTURE_2D,
                    GL_TEXTURE_MAX_ANISOTROPY_EXT,
                    math::clamp(0.0f, 8.0f, maxAnisotropy));
}

void Texture::generate_texture_units(uint32_t n_units,
                                     uint32_t width,
                                     uint32_t height,
                                     unsigned char** data,
                                     unsigned int* internalFormats,
                                     unsigned int* formats,
                                     const std::vector<TextureFilter>& filters,
                                     TextureWrap wrap_param,
                                     bool lazy_mipmap)
{
    n_units_ = n_units;
#ifdef __PROFILING_SET_2x2_TEXTURE__
    width_  = 2;
    height_ = 2;
#else
    width_  = width;
    height_ = height;
#endif

    texture_ids_ = new GLuint[n_units];

    // Generate the right amount of texture units
    glGenTextures(n_units, texture_ids_);
    // For each unit
    for(uint32_t ii = 0; ii < n_units; ++ii)
    {
        // Bind unit
        glBindTexture(GL_TEXTURE_2D, texture_ids_[ii]);

        // Generate filter and check whether this unit has mipmaps
        bool has_mipmap = handle_filter(filters[ii], GL_TEXTURE_2D);
        // Set wrap parameters
        handle_addressUV(wrap_param, GL_TEXTURE_2D);
        // Check whether this unit has depth information
        is_depth_.push_back((formats[ii] == GL_DEPTH_COMPONENT ||
                             formats[ii] == GL_DEPTH_STENCIL));
        // Get data type relative to internal format
        GLenum dataType = internal_format_to_data_type(internalFormats[ii]);

        // Specify OpenGL texture
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     internalFormats[ii],
                     width_,
                     height_,
                     0,
                     formats[ii],
                     dataType,
                     (data)?data[ii]:nullptr);

        // Handle mipmap if specified
        if(has_mipmap && !lazy_mipmap)
            generate_mipmaps(ii);
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        }
    }
}

}
