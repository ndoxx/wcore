#include <cassert>
#include <vector>
#include <sstream>
#include <GL/glew.h>


#include "texture.h"
#include "material_common.h"
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

static const uint32_t SAMPLER_GROUP_SIZE = 3;
static std::vector<std::map<TextureBlock, hash_t>> SAMPLER_NAMES =
{
    // Sampler names for sampler group 1
    {
        {TextureBlock::BLOCK0, "mt.sg1.block0Tex"_h},
        {TextureBlock::BLOCK1, "mt.sg1.block1Tex"_h},
        {TextureBlock::BLOCK2, "mt.sg1.block2Tex"_h}
    },

    // Sampler names for sampler group 2
    {
        {TextureBlock::BLOCK0, "mt.sg2.block0Tex"_h},
        {TextureBlock::BLOCK1, "mt.sg2.block1Tex"_h},
        {TextureBlock::BLOCK2, "mt.sg2.block2Tex"_h}
    }
};

static std::map<TextureIF, GLenum> DATA_TYPES =
{
    {TextureIF::SRGB_ALPHA,        GL_FLOAT},
    {TextureIF::RGB16F,            GL_FLOAT},
    {TextureIF::RGBA16F,           GL_FLOAT},
    {TextureIF::RG16_SNORM,        GL_UNSIGNED_BYTE},
    {TextureIF::RGB16_SNORM,       GL_UNSIGNED_BYTE},
    {TextureIF::DEPTH32F_STENCIL8, GL_FLOAT_32_UNSIGNED_INT_24_8_REV},
    {TextureIF::DEPTH24_STENCIL8,  GL_UNSIGNED_INT_24_8},
    {TextureIF::RGB8,              GL_BYTE},
    {TextureIF::R8,                GL_BYTE},
};

static std::map<TextureIF, GLenum> INTERNAL_FORMATS =
{
    {TextureIF::R8,                              GL_R8},
    {TextureIF::RGB8,                            GL_RGB8},
    {TextureIF::RGBA8,                           GL_RGBA8},
    {TextureIF::RGB16F,                          GL_RGB16F},
    {TextureIF::RGBA16F,                         GL_RGBA16F},
    {TextureIF::RGB32F,                          GL_RGB32F},
    {TextureIF::RGBA32F,                         GL_RGBA32F},
    {TextureIF::SRGB_ALPHA,                      GL_SRGB_ALPHA},
    {TextureIF::RG16_SNORM,                      GL_RG16_SNORM},
    {TextureIF::RGB16_SNORM,                     GL_RGB16_SNORM},
    {TextureIF::RGBA16_SNORM,                    GL_RGBA16_SNORM},
    {TextureIF::COMPRESSED_RGB_S3TC_DXT1,        GL_COMPRESSED_RGB_S3TC_DXT1_EXT},
    {TextureIF::COMPRESSED_RGBA_S3TC_DXT1,       GL_COMPRESSED_RGBA_S3TC_DXT1_EXT},
    {TextureIF::COMPRESSED_RGBA_S3TC_DXT3,       GL_COMPRESSED_RGBA_S3TC_DXT3_EXT},
    {TextureIF::COMPRESSED_RGBA_S3TC_DXT5,       GL_COMPRESSED_RGBA_S3TC_DXT5_EXT},
    {TextureIF::COMPRESSED_SRGB_S3TC_DXT1,       GL_COMPRESSED_SRGB_S3TC_DXT1_EXT},
    {TextureIF::COMPRESSED_SRGB_ALPHA_S3TC_DXT1, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT},
    {TextureIF::COMPRESSED_SRGB_ALPHA_S3TC_DXT3, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT},
    {TextureIF::COMPRESSED_SRGB_ALPHA_S3TC_DXT5, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT},
    {TextureIF::DEPTH_COMPONENT16,               GL_DEPTH_COMPONENT16},
    {TextureIF::DEPTH_COMPONENT24,               GL_DEPTH_COMPONENT24},
    {TextureIF::DEPTH_COMPONENT32F,              GL_DEPTH_COMPONENT32F},
    {TextureIF::DEPTH24_STENCIL8,                GL_DEPTH24_STENCIL8},
    {TextureIF::DEPTH32F_STENCIL8,               GL_DEPTH32F_STENCIL8},
};

static std::map<TextureF, GLenum> FORMATS =
{
    {TextureF::RED,               GL_RED},
    {TextureF::RGB,               GL_RGB},
    {TextureF::RGBA,              GL_RGBA},
    {TextureF::DEPTH_COMPONENT,   GL_DEPTH_COMPONENT},
    {TextureF::DEPTH_STENCIL,     GL_DEPTH_STENCIL},
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

static GLenum internal_format_to_data_type(TextureIF iformat)
{
    auto it = DATA_TYPES.find(iformat);
    if(it!=DATA_TYPES.end())
        return it->second;
    return GL_UNSIGNED_BYTE;
}

static TextureIF fix_internal_format(TextureIF iformat, hash_t sampler_name)
{
    // Load Albedo / Diffuse textures as sRGB to avoid double gamma-correction.
    if(sampler_name == "mt.sg1.block0Tex"_h || sampler_name == "mt.sg2.block0Tex"_h)
        return TextureIF::COMPRESSED_SRGB_ALPHA_S3TC_DXT1;
    // Do not use DXT1 compression on normal-depth texure (block1) because it will screw up the normals
    else if(sampler_name == "mt.sg1.block1Tex"_h || sampler_name == "mt.sg2.block1Tex"_h)
        return TextureIF::COMPRESSED_RGBA_S3TC_DXT3;//TextureIF::RGBA8;

    return iformat;
}

TextureParameters::TextureParameters():
filter(TextureFilter(TextureFilter::MAG_LINEAR | TextureFilter::MIN_LINEAR_MIPMAP_LINEAR)),
internal_format(TextureIF::COMPRESSED_RGBA_S3TC_DXT1),
format(TextureF::RGBA),
wrap(TextureWrap::REPEAT),
lazy_mipmap(false)
{

}

TextureDescriptor::TextureDescriptor():
unit_flags(0),
sampler_group(1),
parameters(),
resource_id(""_h),
is_wat(false),
owns_data(false),
width(0),
height(0),
block0_data(nullptr),
block1_data(nullptr),
block2_data(nullptr)
{

}

TextureDescriptor::~TextureDescriptor()
{
    release_data();
}

void TextureDescriptor::release_data()
{
    if(owns_data)
    {
        delete[] block0_data;
        delete[] block1_data;
        delete[] block2_data;
        owns_data = false;
    }
}

TextureUnitInfo::TextureUnitInfo(hash_t sampler_name,
                                 TextureFilter filter,
                                 TextureIF internal_format,
                                 TextureF format,
                                 unsigned char* data):
sampler_name_(sampler_name),
filter_(filter),
internal_format_(internal_format),
format_(format),
data_(data),
is_shared_(false)
{

}

TextureUnitInfo::TextureUnitInfo(hash_t sampler_name,
                                 uint32_t texture_id,
                                 UnitType unit_type):
sampler_name_(sampler_name),
is_shared_(true),
texture_id_(texture_id),
unit_type_(unit_type)
{

}

Texture::Texture(const TextureDescriptor& descriptor):
n_units_(0)
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
    width_  = descriptor.width;
    height_ = descriptor.height;

    uint32_t ii=0;
    for(auto&& [key, sampler_name]: SAMPLER_NAMES[sampler_group_-1])
    {
        if(descriptor.has_block(key))
        {
            // Register a sampler name for each block
            block_to_sampler_[key] = uniform_sampler_names_.size() + (sampler_group_-1)*SAMPLER_GROUP_SIZE;

            generate_texture_unit(TextureUnitInfo(sampler_name,
                                                  descriptor.parameters.filter,
                                                  fix_internal_format(descriptor.parameters.internal_format, sampler_name),
                                                  descriptor.parameters.format,
                                                  data_ptrs[ii]),
                                  descriptor.parameters.wrap, false);
        }
        ++ii;
    }
}

Texture::Texture(std::istream& stream):
n_units_(0),
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

    width_  = px_buf->get_width();
    height_ = px_buf->get_height();

    generate_texture_unit(TextureUnitInfo(SAMPLER_NAMES[0][TextureBlock::BLOCK0],
                                          TextureFilter::MIN_LINEAR,
                                          TextureIF::RGBA8,
                                          TextureF::RGB,
                                          px_buf->get_data_pointer()),
                          TextureWrap::CLAMP_TO_EDGE, false);


    delete px_buf;
}

Texture::Texture(std::initializer_list<TextureUnitInfo> units,
                 uint32_t width,
                 uint32_t height,
                 TextureWrap wrap_param,
                 bool lazy_mipmap):
n_units_(0),
unit_flags_(0),
sampler_group_(1)
{
#ifdef __PROFILING_SET_2x2_TEXTURE__
    width_  = 2;
    height_ = 2;
#else
    width_  = width;
    height_ = height;
#endif

    for(auto&& unit: units)
        generate_texture_unit(unit, wrap_param, lazy_mipmap);
}

Texture::~Texture()
{
    if(texture_ids_.size())
        glDeleteTextures(n_units_, &texture_ids_[0]);
}

void Texture::bind_sampler(const Shader& shader, TextureBlock block) const
{
    shader.send_uniform<int>(SAMPLER_NAMES[sampler_group_-1].at(block), block_to_sampler_.at(block));
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

void Texture::generate_texture_unit(const TextureUnitInfo& unit_info,
                                    TextureWrap wrap_param,
                                    bool lazy_mipmap)
{
    if(!unit_info.is_shared_)
    {
        size_t index = texture_ids_.size();
        // Generate one texture unit
        GLuint tex_id;
        glGenTextures(1, &tex_id);
        texture_ids_.push_back(tex_id);
        // Bind unit
        glBindTexture(GL_TEXTURE_2D, texture_ids_[index]);
        // Generate filter and check whether this unit has mipmaps
        bool has_mipmap = handle_filter(unit_info.filter_, GL_TEXTURE_2D);
        // Set wrap parameters
        handle_addressUV(wrap_param, GL_TEXTURE_2D);
        // Register sampler name
        uniform_sampler_names_.push_back(unit_info.sampler_name_);

        // Check whether this unit has depth / stencil information
        UnitType unit_type = UnitType::COLOR;
        if(unit_info.format_ == TextureF::DEPTH_COMPONENT)
            unit_type = UnitType::DEPTH;
        else if(unit_info.format_ == TextureF::DEPTH_STENCIL)
            unit_type = UnitType((uint8_t)UnitType::DEPTH | (uint8_t)UnitType::STENCIL);

        unit_types_.push_back(unit_type);

        // Get data type relative to internal format
        GLenum dataType = internal_format_to_data_type(unit_info.internal_format_);

        // Specify OpenGL texture
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     INTERNAL_FORMATS[unit_info.internal_format_],
                     width_,
                     height_,
                     0,
                     FORMATS[unit_info.format_],
                     dataType,
                     unit_info.data_);

        // Handle mipmap if specified
        if(has_mipmap && !lazy_mipmap)
            generate_mipmaps(index);
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        }
    }
    else // Texture unit is shared and has already been generated and configured by another Texture
    {
        texture_ids_.push_back(unit_info.texture_id_);
        uniform_sampler_names_.push_back(unit_info.sampler_name_);
        unit_types_.push_back(unit_info.unit_type_);
    }

    ++n_units_;
}

TextureUnitInfo Texture::share_unit(uint32_t index)
{
    assert(index<n_units_ && "Texture::share_unit() index out of bounds.");
    return TextureUnitInfo(uniform_sampler_names_.at(index), texture_ids_.at(index), unit_types_.at(index));
}


}
