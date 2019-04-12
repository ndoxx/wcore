#include <cassert>
#include <vector>
#include <sstream>


#include "texture.h"
#include "pixel_buffer.h"
#include "logger.h"
#include "algorithms.h"
#include "material_common.h"
#include "config.h"
#include "file_system.h"
#include "error.h"

namespace wcore
{

uint32_t Texture::TextureInternal::Ninst = 0;
Texture::RMap Texture::RESOURCE_MAP_;
Texture::TMap Texture::NAMED_TEXTURES_;
/*
std::map<TextureUnit, hash_t> Texture::SAMPLER_NAMES_ =
{
    {TextureUnit::ALBEDO,    "mt.sg1.albedoTex"_h},
    {TextureUnit::AO,        "mt.sg1.AOTex"_h},
    {TextureUnit::DEPTH,     "mt.sg1.depthTex"_h},
    {TextureUnit::METALLIC,  "mt.sg1.metallicTex"_h},
    {TextureUnit::NORMAL,    "mt.sg1.normalTex"_h},
    {TextureUnit::ROUGHNESS, "mt.sg1.roughnessTex"_h}
};

// Alternative sampler group for splat mapping
std::map<TextureUnit, hash_t> Texture::SAMPLER_NAMES_2_ =
{
    {TextureUnit::ALBEDO,    "mt.sg2.albedoTex"_h},
    {TextureUnit::AO,        "mt.sg2.AOTex"_h},
    {TextureUnit::DEPTH,     "mt.sg2.depthTex"_h},
    {TextureUnit::METALLIC,  "mt.sg2.metallicTex"_h},
    {TextureUnit::NORMAL,    "mt.sg2.normalTex"_h},
    {TextureUnit::ROUGHNESS, "mt.sg2.roughnessTex"_h}
};

static uint32_t SAMPLER_GROUP_SIZE = 6;*/

std::map<TextureUnit, hash_t> Texture::SAMPLER_NAMES_ =
{
    {TextureUnit::BLOCK0, "mt.sg1.block0Tex"_h},
    {TextureUnit::BLOCK1, "mt.sg1.block1Tex"_h},
    {TextureUnit::BLOCK2, "mt.sg1.block2Tex"_h},
};
std::map<TextureUnit, hash_t> Texture::SAMPLER_NAMES_2_ =
{
    {TextureUnit::BLOCK0, "mt.sg2.block0Tex"_h},
    {TextureUnit::BLOCK1, "mt.sg2.block1Tex"_h},
    {TextureUnit::BLOCK2, "mt.sg2.block2Tex"_h},
};

static uint32_t SAMPLER_GROUP_SIZE = 3;

PngLoader Texture::png_loader_;

static std::map<GLenum, GLenum> DATA_TYPES =
{
    {GL_SRGB_ALPHA, GL_FLOAT},
    {GL_RGB16F, GL_FLOAT},
    {GL_RGBA16F, GL_FLOAT},
    {GL_RG16_SNORM, GL_UNSIGNED_BYTE},
    {GL_RGB16_SNORM, GL_UNSIGNED_BYTE},
    {GL_DEPTH32F_STENCIL8, GL_FLOAT_32_UNSIGNED_INT_24_8_REV},
    {GL_DEPTH24_STENCIL8, GL_UNSIGNED_INT_24_8},
    {GL_RGB8, GL_BYTE},
    {GL_R8, GL_BYTE},
};

const std::map<TextureUnit, hash_t>& Texture::select_sampler_group(uint8_t group)
{
    if(group == 1)
        return Texture::SAMPLER_NAMES_;
    else //if(group == 2)
        return Texture::SAMPLER_NAMES_2_;
}

static GLenum internal_format_to_data_type(GLenum iformat)
{
    auto it = DATA_TYPES.find(iformat);
    if(it!=DATA_TYPES.end())
        return it->second;
    return GL_UNSIGNED_BYTE;
}

static bool handle_filter(GLenum filter, GLenum target)
{
    bool has_mipmap = (filter == GL_NEAREST_MIPMAP_NEAREST ||
                       filter == GL_NEAREST_MIPMAP_LINEAR ||
                       filter == GL_LINEAR_MIPMAP_NEAREST ||
                       filter == GL_LINEAR_MIPMAP_LINEAR);

    // Set filter
    if(filter != GL_NONE)
    {
        glTexParameterf(target, GL_TEXTURE_MIN_FILTER, filter);
        if(!has_mipmap)
            glTexParameterf(target, GL_TEXTURE_MAG_FILTER, filter);
        else
            glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    return has_mipmap;
}

// TMP
static void handle_addressUV(bool clamp, GLenum target)
{
    if(clamp)
    {
        glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
}

static void handle_mipmap(bool bound_tex_has_mipmap, GLenum target)
{
    if(bound_tex_has_mipmap)
    {
        glGenerateMipmap(target);
        GLfloat maxAnisotropy;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
        glTexParameterf(target,
                        GL_TEXTURE_MAX_ANISOTROPY_EXT,
                        math::clamp(0.0f, 8.0f, maxAnisotropy));
    }
    else
    {
        glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0);
    }
}

#ifdef __DEBUG__
void Texture::debug_print_rmap_bindings()
{
    DLOG("[Texture] Displaying binding states: ", "texture", Severity::DET);
    for(auto&& [hash, pinternal]: RESOURCE_MAP_)
    {
        auto && states = pinternal->get_binding_states();
        std::cout << hash << " :" << std::endl;
        for(auto&& [texID, activeTexID]: states)
            std::cout << "  -> id: " << texID << " active: " << activeTexID << std::endl;
    }
}
#endif

void Texture::register_named_texture(hash_t name, pTexture ptex)
{
    #if __DEBUG__
    {
        std::stringstream ss;
        ss << "[Texture] Registering new named texture: "
           << "<n>" << HRESOLVE(name) << "</n>";
        DLOGN(ss.str(), "texture");
        DLOGI("width:  <v>" + std::to_string(ptex->get_width()) + "</v>", "texture");
        DLOGI("height: <v>" + std::to_string(ptex->get_height()) + "</v>", "texture");
        DLOGI("units:  <v>" + std::to_string(ptex->get_num_textures()) + "</v>", "texture");

    }
    #endif
    auto it = NAMED_TEXTURES_.find(name);
    if(it == NAMED_TEXTURES_.end())
    {
        NAMED_TEXTURES_[name] = ptex;
    }
    else
    {
        #if __DEBUG__
            std::stringstream ss;
            ss << "[Texture] Ignored duplicate named texture registration for: <n>" << name << "</n>";
            DLOGW(ss.str(), "texture");
        #endif
    }
}

Texture::wpTexture Texture::get_named_texture(hash_t name)
{
    auto it = NAMED_TEXTURES_.find(name);
    if(it == NAMED_TEXTURES_.end())
    {
        std::stringstream ss;
        ss << "[Texture] Couldn't find named texture: <n>" << name << "</n>";
        DLOGF(ss.str(), "texture");
        fatal("Couldn't find named texture.");
        return wpTexture(); // Never gets here
    }
    else
        return wpTexture(it->second);
}

Texture::TextureInternal::TextureInternal(const TextureDescriptor& descriptor):
textureTarget_(GL_TEXTURE_2D),
numTextures_(descriptor.locations.size()),
ID_(++Ninst)
{
    unsigned char** data    = new unsigned char*[numTextures_];
    PixelBuffer** px_bufs   = new PixelBuffer*[numTextures_];
    GLenum* filters         = new GLenum[numTextures_];
    GLenum* internalFormats = new GLenum[numTextures_];
    GLenum* formats         = new GLenum[numTextures_];

    uint32_t ii=0;
    for(auto&& [key, sampler_name]: Texture::select_sampler_group(descriptor.sampler_group))
    {
        if(!descriptor.has_unit(key))
            continue;

        filters[ii] = descriptor.parameters.filter;
        if(sampler_name == "mt.sg1.block0Tex"_h || sampler_name == "mt.sg2.block0Tex"_h)
        {
            // Load Albedo / Diffuse textures as sRGB to avoid
            // double gamma-correction.
            internalFormats[ii] = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
            //internalFormats[ii] = GL_SRGB_ALPHA;
        }
        else if(sampler_name == "mt.sg1.block1Tex"_h || sampler_name == "mt.sg2.block1Tex"_h)
        {
            // Do not use DXT1 compression on normal-depth texure (block1) because
            // it will screw up the normals
            internalFormats[ii] = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;//GL_RGBA;
        }
        else
        {
            internalFormats[ii] = descriptor.parameters.internal_format;
        }
        formats[ii] = descriptor.parameters.format;

        auto stream = FILESYSTEM.get_file_as_stream(descriptor.locations.at(key).c_str(), "root.folders.texture"_h, "pack0"_h);
        px_bufs[ii] = png_loader_.load_png(*stream);

        if(px_bufs[ii])
        {
            data[ii] = px_bufs[ii]->get_data_pointer();
            #if __DEBUG__
                DLOGN("[PixelBuffer] <z>[" + std::to_string(ii) + "]</z>", "texture");
                if(dbg::LOG.get_channel_verbosity("texture"_h) == 3u)
                    px_bufs[ii]->debug_display();
                    //std::cout << *px_bufs[ii] << std::endl;
            #endif
        }
        else
        {
            DLOGF("[Texture] Unable to load Texture.", "texture");
            for (uint32_t jj=0; jj<=ii; ++jj)
                if(px_bufs[jj])
                    delete px_bufs[jj];
            delete [] formats;
            delete [] internalFormats;
            delete [] filters;
            delete [] data;
            delete [] px_bufs;
            fatal();
        }
        ++ii;
    }

    // Get texture size
#ifdef __PROFILING_SET_2x2_TEXTURE__
    width_  = 2;
    height_ = 2;
#else
    width_  = px_bufs[0]->get_width();
    height_ = px_bufs[0]->get_height();
#endif

    // * GL calls
    // Generate the right amount of textures
    textureID_ = new GLuint[numTextures_];
    is_depth_  = new bool[numTextures_];
    glGenTextures(numTextures_, textureID_);
    // For each texture in there
    for(uint32_t ii = 0; ii < numTextures_; ++ii)
    {
        bind(ii);
        // Generate filter and check whether this unit has mipmaps
        bool has_mipmap = handle_filter(filters[ii], textureTarget_);
        // Set clamp/wrap parameters
        handle_addressUV(descriptor.parameters.clamp, textureTarget_);
        // Check whether this unit has depth information
        is_depth_[ii] = (formats[ii] == GL_DEPTH_COMPONENT ||
                         formats[ii] == GL_DEPTH_STENCIL);
        // Get data type relative to internal format
        GLenum dataType = internal_format_to_data_type(internalFormats[ii]);

        // Specify OpenGL texture
        glTexImage2D(textureTarget_,
                     0,
                     internalFormats[ii],
                     width_,
                     height_,
                     0,
                     formats[ii],
                     dataType,
                     (data)?data[ii]:nullptr);

        // Handle mipmap if specified
        handle_mipmap(has_mipmap, textureTarget_);
    }

    // Free allocations
    for (uint32_t jj=0; jj<numTextures_; ++jj)
        if(px_bufs[jj])
            delete px_bufs[jj];
    delete [] formats;
    delete [] internalFormats;
    delete [] filters;
    delete [] data;
    delete [] px_bufs;
}

Texture::TextureInternal::TextureInternal(std::istream& stream):
textureTarget_(GL_TEXTURE_2D),
numTextures_(1),
ID_(++Ninst)
{
    if(!stream.good())
    {
        DLOGE("[Texture] Failed to create texture from stream: stream is bad.", "texture");
        return;
    }

    PixelBuffer* px_buf = png_loader_.load_png(stream);

#if __DEBUG__
    DLOGN("[PixelBuffer]", "texture");
    if(dbg::LOG.get_channel_verbosity("texture"_h) == 3u)
        px_buf->debug_display();
#endif

    textureID_ = new GLuint[1];
    is_depth_  = new bool[1];
    is_depth_[0] = false;
    glGenTextures(1, textureID_);
    glBindTexture(textureTarget_, textureID_[0]);

    handle_filter(GL_LINEAR, textureTarget_);
    handle_addressUV(true, textureTarget_);

    // Specify OpenGL texture
    glTexImage2D(textureTarget_,
                 0,
                 GL_RGB,
                 px_buf->get_width(),
                 px_buf->get_height(),
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 px_buf->get_data_pointer());

    delete px_buf;
}

Texture::TextureInternal::TextureInternal(GLenum textureTarget,
                                          uint32_t numTextures,
                                          uint32_t width,
                                          uint32_t height,
                                          unsigned char** data,
                                          GLenum* filters,
                                          GLenum* internalFormats,
                                          GLenum* formats,
                                          bool clamp,
                                          bool lazy_mipmap):
textureTarget_(textureTarget),
numTextures_(numTextures),
#ifdef __PROFILING_SET_2x2_TEXTURE__
    width_(2),
    height_(2),
#else
    width_(width),
    height_(height),
#endif
ID_(++Ninst)
{
    textureID_ = new GLuint[numTextures_];
    is_depth_  = new bool[numTextures_];

    // Generate the right amount of texture units
    glGenTextures(numTextures_, textureID_);
    // For each unit
    for(uint32_t ii = 0; ii < numTextures_; ++ii)
    {
        // Bind unit
        bind(ii);
        // Generate filter and check whether this unit has mipmaps
        bool has_mipmap = handle_filter(filters[ii], textureTarget_);
        // Set clamp/wrap parameters
        handle_addressUV(clamp, textureTarget_);
        // Check whether this unit has depth information
        is_depth_[ii] = (formats[ii] == GL_DEPTH_COMPONENT ||
                         formats[ii] == GL_DEPTH_STENCIL);
        // Get data type relative to internal format
        GLenum dataType = internal_format_to_data_type(internalFormats[ii]);

        // Specify OpenGL texture
        glTexImage2D(textureTarget_,
                     0,
                     internalFormats[ii],
                     width_,
                     height_,
                     0,
                     formats[ii],
                     dataType,
                     (data)?data[ii]:nullptr);

        // Handle mipmap if specified
        handle_mipmap(has_mipmap && !lazy_mipmap, textureTarget_);
    }
}


Texture::TextureInternal::~TextureInternal()
{
    if(*textureID_) glDeleteTextures(numTextures_, textureID_);
    if(textureID_) delete[] textureID_;
    if(is_depth_) delete[] is_depth_;
}

void Texture::TextureInternal::bind(GLuint textureNum) const
{
    glBindTexture(textureTarget_, textureID_[textureNum]);
}

Texture::Texture(const std::vector<hash_t>& sampler_names,
                 uint32_t width,
                 uint32_t height,
                 GLenum textureTarget,
                 GLenum filter,
                 GLenum internalFormat,
                 GLenum format,
                 bool clamp,
                 bool lazy_mipmap):
resourceID_(H_("")),
units_(0),
sampler_group_(1),
uniform_sampler_names_(sampler_names)
{
    GLenum* filters         = new GLenum[sampler_names.size()];
    GLenum* internalFormats = new GLenum[sampler_names.size()];
    GLenum* formats         = new GLenum[sampler_names.size()];
    for(uint32_t ii=0; ii<sampler_names.size(); ++ii)
    {
        filters[ii] = filter;
        internalFormats[ii] = internalFormat;
        formats[ii] = format;
    }

    internal_ = std::make_shared<TextureInternal>(textureTarget,
                                                  sampler_names.size(),
                                                  width,
                                                  height,
                                                  nullptr,
                                                  filters,
                                                  internalFormats,
                                                  formats,
                                                  clamp,
                                                  lazy_mipmap);
    delete [] filters;
    delete [] internalFormats;
    delete [] formats;
}

Texture::Texture(const std::vector<hash_t>& sampler_names,
                 const std::vector<GLenum>& filters,
                 const std::vector<GLenum>& internalFormats,
                 const std::vector<GLenum>& formats,
                 uint32_t width,
                 uint32_t height,
                 GLenum textureTarget,
                 bool clamp,
                 bool lazy_mipmap):
internal_(new TextureInternal(textureTarget,
                              sampler_names.size(),
                              width,
                              height,
                              nullptr,
                              (GLenum*)&filters[0],
                              (GLenum*)&internalFormats[0],
                              (GLenum*)&formats[0],
                              clamp,
                              lazy_mipmap)),
resourceID_(H_("")),
units_(0),
sampler_group_(1),
uniform_sampler_names_(sampler_names){}

Texture::Texture(const TextureDescriptor& descriptor):
resourceID_(descriptor.resource_id),
units_(descriptor.units)
{
    #if __DEBUG__
    {
        std::stringstream ss;
        ss << "[Texture] New texture from asset: <n>" << HRESOLVE(resourceID_) << "</n>";
        DLOGN(ss.str(), "texture");
    }
    #endif


    // Try to find a cached version first
    auto it = RESOURCE_MAP_.find(resourceID_);
    bool cache_exists = (it != RESOURCE_MAP_.end());

    // Register a sampler name for each unit
    sampler_group_ = descriptor.sampler_group;
    for(auto&& [key, sampler_name]: Texture::select_sampler_group(descriptor.sampler_group))
    {
        if(descriptor.has_unit(key))
        {
            unit_indices_[key] = uniform_sampler_names_.size() + (descriptor.sampler_group-1)*SAMPLER_GROUP_SIZE;
            uniform_sampler_names_.push_back(sampler_name);

            #if __DEBUG__
                if(!cache_exists)
                {
                    std::stringstream ss;
                    ss << "<v>" << sampler_name << "</v> <- <p>"
                       << descriptor.locations.at(key) << "</p>";
                    DLOGI(ss.str(), "texture");
                }
            #endif
        }
    }

    // If cached version exists, use it.
    if(cache_exists)
    {
        internal_ = it->second;
        #if __DEBUG__
            DLOGI("<i>Using cache.</i>", "texture");
        #endif
    }
    // Else, create new texture internal using parameters
    else
    {
        internal_ = std::make_shared<TextureInternal>(descriptor);

        // Cache resource for later if texture is meant to be cached
        if(resourceID_ != ""_h)
            RESOURCE_MAP_.insert(std::make_pair(resourceID_, internal_));
    }
}

Texture::Texture(std::istream& stream):
resourceID_(""_h),
units_(1)
{
    internal_ = std::make_shared<TextureInternal>(stream);
}


Texture::~Texture()
{
    // If only the object which caused destruction of Texture
    // and the resource map hold a reference to texture,
    // no need to cache texture anymore
    if(internal_.use_count() == 2)
    {
        if(resourceID_ != ""_h)
        {
            RESOURCE_MAP_.erase(resourceID_);
            #ifdef __DEBUG__
            std::stringstream ss;
            ss << "[Texture] Destroying cached texture: <n>" << HRESOLVE(resourceID_) << "</n>";
            DLOGN(ss.str(), "texture");
            #endif
        }
    }
}

void Texture::bind(GLuint unit) const
{
    assert(unit >= 0 && unit <= 31);
    glActiveTexture(GL_TEXTURE0 + unit + (sampler_group_-1)*SAMPLER_GROUP_SIZE);
    internal_->bind(unit);

#ifdef __DEBUG__
    internal_->set_binding_state(unit, unit);
#endif
}

void Texture::bind(GLuint unit, uint32_t index) const
{
    assert(unit >= 0 && unit <= 31);
    assert(index >= 0 && index < internal_->get_num_textures());
    glActiveTexture(GL_TEXTURE0 + unit);
    internal_->bind(index);

#ifdef __DEBUG__
    internal_->set_binding_state(index, unit);
#endif
}

void Texture::bind_all() const
{
    for(GLuint ii=0; ii<internal_->get_num_textures(); ++ii)
        bind(ii);
}

void Texture::unbind() const
{
    for(GLuint ii=0; ii<internal_->get_num_textures(); ++ii)
    {
        glActiveTexture(GL_TEXTURE0 + ii);
        glBindTexture(GL_TEXTURE_2D, 0);
#ifdef __DEBUG__
        internal_->set_binding_state(ii, -1);
#endif
    }
}

void Texture::generate_mipmaps(uint32_t unit,
                               uint32_t base_level,
                               uint32_t max_level) const
{
    internal_->bind(unit);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, base_level);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, max_level);
    glGenerateMipmap(GL_TEXTURE_2D);
    GLfloat maxAnisotropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
    glTexParameterf(GL_TEXTURE_2D,
                    GL_TEXTURE_MAX_ANISOTROPY_EXT,
                    math::clamp(0.0f, 8.0f, maxAnisotropy));
}

bool Texture::operator==(const Texture& texture) const
{
    return internal_->ID() == texture.internal_->ID();
}

bool Texture::operator!=(const Texture& texture) const
{
    return !operator==(texture);
}

}
