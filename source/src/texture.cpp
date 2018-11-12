#include <cassert>
#include <stdexcept>
#include <filesystem>
#include <vector>
namespace fs = std::filesystem;

#include "texture.h"
#include "png_loader.h"
#include "pixel_buffer.h"
#include "logger.h"
#include "algorithms.h"
#include "material_factory.h"

uint32_t Texture::TextureInternal::Ninst = 0;
Texture::RMap Texture::RESOURCE_MAP_;
//Texture::AMap Texture::ASSET_MAP_;
Texture::TMap Texture::NAMED_TEXTURES_;
const std::vector<std::string> Texture::W_MANDATORY_SAMPLERS_{"mt.albedoTex", "mt.metallicTex", "mt.AOTex", "mt.roughnessTex"};
const std::string Texture::TEX_IMAGE_PATH("../res/textures/");

#ifdef __DEBUG_TEXTURE__
void Texture::debug_print_rmap_bindings()
{
    DLOG("[Texture] Displaying binding states: ");
    for(auto&& [hash, pinternal]: RESOURCE_MAP_)
    {
        auto && states = pinternal->get_binding_states();
        std::cout << hash << " :" << std::endl;
        for(auto&& [texID, activeTexID]: states)
            std::cout << "  -> id: " << texID << " active: " << activeTexID << std::endl;
    }
}
#endif

static std::string extract_texture_sampler_name(const std::string& fileName)
{
    auto const pos_dot = fileName.find_last_of('.');
    auto const pos_und = fileName.find_last_of('_');
    return fileName.substr(pos_und+1, pos_dot-pos_und-1);
}

static std::string extract_texture_asset_name(const std::string& fileName)
{
    auto const pos_und = fileName.find_last_of('_');
    auto const pos_slh = fileName.find_last_of('/');
    return fileName.substr(pos_slh+1, pos_und-pos_slh-1);
}
/*
void Texture::load_asset_map()
{
    DLOGN("[Texture] Loading texture paths.");
    // Iterate over files in texture directory, find files containing
    // the asset name
    for (auto & p : fs::directory_iterator(TEX_IMAGE_PATH))
    {
        if(!is_directory(p))
        {
            std::string fileName(p.path().c_str());
            std::string assetName = extract_texture_asset_name(fileName);

            hash_t hashname = H_(assetName.c_str());
            AMap::iterator it = ASSET_MAP_.find(hashname);
            if(it != ASSET_MAP_.end())
                it->second.push_back(fileName);
            else
                ASSET_MAP_.insert(std::make_pair(hashname,std::vector<std::string>{fileName}));
        }
    }
}*/

void Texture::register_named_texture(hash_t name, pTexture ptex)
{
    #if __DEBUG_TEXTURE_VERBOSE__
    {
        std::stringstream ss;
        ss << "[Texture] Registering new named texture: "
           << "<n>" << name << "</n>";
        DLOGN(ss.str());
        DLOGI("width:  <v>" + std::to_string(ptex->get_width()) + "</v>");
        DLOGI("height: <v>" + std::to_string(ptex->get_height()) + "</v>");
        DLOGI("units:  <v>" + std::to_string(ptex->get_num_textures()) + "</v>");

    }
    #endif
    auto it = NAMED_TEXTURES_.find(name);
    if(it == NAMED_TEXTURES_.end())
    {
        NAMED_TEXTURES_[name] = ptex;
    }
    else
    {
        #if __DEBUG_TEXTURE_VERBOSE__
            std::stringstream ss;
            ss << "[Texture] Ignored duplicate named texture registration for: <n>" << name << "</n>";
            DLOGW(ss.str());
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
        DLOGF(ss.str());
        throw std::runtime_error("Couldn't find named texture.");
    }
    else
        return wpTexture(it->second);
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

    // Generate the right amount of textures
    glGenTextures(numTextures_, textureID_);
    // For each texture in there
    for(uint32_t ii = 0; ii < numTextures_; ++ii)
    {
        bind(ii);
        bool has_mipmap = (filters[ii] == GL_NEAREST_MIPMAP_NEAREST ||
                           filters[ii] == GL_NEAREST_MIPMAP_LINEAR ||
                           filters[ii] == GL_LINEAR_MIPMAP_NEAREST ||
                           filters[ii] == GL_LINEAR_MIPMAP_LINEAR);

        // Set filter
        if(filters[ii] != GL_NONE)
        {
            glTexParameterf(textureTarget_, GL_TEXTURE_MIN_FILTER, filters[ii]);
            if(!has_mipmap)
                glTexParameterf(textureTarget_, GL_TEXTURE_MAG_FILTER, filters[ii]);
            else
                glTexParameterf(textureTarget_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        // Set clamp parameters if needed
        if(clamp)
        {
            glTexParameterf(textureTarget_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(textureTarget_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        is_depth_[ii] = (formats[ii] == GL_DEPTH_COMPONENT ||
                         formats[ii] == GL_DEPTH_STENCIL);


        GLenum dataType;
        if(internalFormats[ii] == GL_RGB16F ||
           internalFormats[ii] == GL_RGBA16F)
        {
            dataType = GL_FLOAT;
        }
        else if(internalFormats[ii] == GL_RG16_SNORM ||
                internalFormats[ii] == GL_RGB16_SNORM)
        {
            dataType = GL_UNSIGNED_BYTE;
        }
        else if(internalFormats[ii] == GL_DEPTH32F_STENCIL8)
        {
            dataType = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
        }
        else if(internalFormats[ii] == GL_DEPTH24_STENCIL8)
        {
            dataType = GL_UNSIGNED_INT_24_8;
        }
        else
        {
            dataType = GL_UNSIGNED_BYTE;
        }

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

        // Handle mipmaps if specified
        if(has_mipmap && !lazy_mipmap)
        {
            glGenerateMipmap(textureTarget_);
            GLfloat maxAnisotropy;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
            glTexParameterf(textureTarget_,
                            GL_TEXTURE_MAX_ANISOTROPY_EXT,
                            math::clamp(0.0f, 8.0f, maxAnisotropy));
        }
        else
        {
            glTexParameteri(textureTarget_, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(textureTarget_, GL_TEXTURE_MAX_LEVEL, 0);
        }
#if __DEBUG_TEXTURE__
        std::cout << std::endl;
#endif
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

Texture::Texture(const std::vector<std::string>& sampler_names,
                 uint32_t width,
                 uint32_t height,
                 GLenum textureTarget,
                 GLenum filter,
                 GLenum internalFormat,
                 GLenum format,
                 bool clamp,
                 bool lazy_mipmap):
resourceID_(H_("")),
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

Texture::Texture(const std::vector<std::string>& sampler_names,
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
uniform_sampler_names_(sampler_names){}

Texture::Texture(const Texture& texture) :
internal_(texture.internal_),
resourceID_(texture.resourceID_),
uniform_sampler_names_(texture.uniform_sampler_names_){}

Texture::Texture(Texture&& texture):
internal_(std::move(texture.internal_)),
resourceID_(std::move(texture.resourceID_)),
uniform_sampler_names_(std::move(texture.uniform_sampler_names_)){}

#include <sstream>

static std::map<hashstr_t, const char*> SAMPLERS =
{
    {HS_("Albedo"),    "mt.albedoTex"},
    {HS_("AO"),        "mt.AOTex"},
    {HS_("Depth"),     "mt.depthTex"},
    {HS_("Metallic"),  "mt.metallicTex"},
    {HS_("Normal"),    "mt.normalTex"},
    {HS_("Roughness"), "mt.roughnessTex"}
};

Texture::Texture(const TextureDescriptor& descriptor):
resourceID_(descriptor.resource_id)
{
    auto& fileNames = descriptor.locations;
    uint32_t numTextures = fileNames.size();

    #if __DEBUG_TEXTURE_VERBOSE__
    {
        std::stringstream ss;
        ss << "[Texture] New texture from asset: <n>" << resourceID_ << "</n>";
        DLOGN(ss.str());
    }
    #endif


    // Try to find a cached version first
    auto it = RESOURCE_MAP_.find(resourceID_);
    bool cache_exists = (it != RESOURCE_MAP_.end());

    for(auto&& [key, sampler_name]: SAMPLERS)
    {
        if(descriptor.has_unit.at(key))
        {
            uniform_sampler_names_.push_back(sampler_name);
            #if __DEBUG_TEXTURE_VERBOSE__
                if(!cache_exists)
                {
                    std::stringstream ss;
                    ss << "<v>" << sampler_name << "</v> <- <p>"
                       << descriptor.locations.at(key) << "</p>";
                    DLOGI(ss.str());
                }
            #endif
        }
    }

    // If cached version exists, use it.
    if(cache_exists)
    {
        internal_ = it->second;
        #if __DEBUG_TEXTURE_VERBOSE__
            DLOGI("<i>Using cache.</i>");
        #endif
    }
    // Else, create new texture internal using parameters
    else
    {
        unsigned char** data    = new unsigned char*[numTextures];
        PixelBuffer** px_bufs   = new PixelBuffer*[numTextures];
        GLenum* filters         = new GLenum[numTextures];
        GLenum* internalFormats = new GLenum[numTextures];
        GLenum* formats         = new GLenum[numTextures];

        uint32_t ii=0;
        for(auto&& [key, sampler_name]: SAMPLERS)
        {
            if(!descriptor.has_unit.at(key))
                continue;

            filters[ii] = descriptor.parameters.filter;
            if(!strcmp(sampler_name, "mt.diffuseTex"))
            {
                // Load Albedo / Diffuse textures as sRGB to avoid
                // double gamma-correction.
                internalFormats[ii] = GL_SRGB_ALPHA;
            }
            else
            {
                internalFormats[ii] = descriptor.parameters.internal_format;
            }
            formats[ii] = descriptor.parameters.format;

            try
            {
                px_bufs[ii] = PngLoader::Instance().load_png(("../res/textures/" + fileNames.at(key)).c_str());
                data[ii] = px_bufs[ii]->get_data_pointer();
                #if __DEBUG_TEXTURE_VERBOSE__
                    DLOGN("[PixelBuffer] <z>[" + std::to_string(ii) + "]</z>");
                    std::cout << *px_bufs[ii] << std::endl;
                #endif
            }
            catch(const std::exception& e)
            {
                DLOGF("[Texture] Unable to load Texture.");
                for (uint32_t jj=0; jj<=ii; ++jj)
                    if(px_bufs[jj])
                        delete px_bufs[jj];
                delete [] formats;
                delete [] internalFormats;
                delete [] filters;
                delete [] data;
                delete [] px_bufs;
                throw;
            }
            ++ii;
        }

        // Create new texture internal
        internal_ = std::make_shared<TextureInternal>(GL_TEXTURE_2D,
                                                      numTextures,
                                                      px_bufs[0]->get_width(),
                                                      px_bufs[0]->get_height(),
                                                      data,
                                                      filters,
                                                      internalFormats,
                                                      formats,
                                                      descriptor.parameters.clamp,
                                                      descriptor.parameters.lazy_mipmap);

        // Free pixel buffers
        for (uint32_t jj=0; jj<numTextures; ++jj)
            if(px_bufs[jj])
                delete px_bufs[jj];
        delete [] formats;
        delete [] internalFormats;
        delete [] filters;
        delete [] data;
        delete [] px_bufs;

        // Cache resource for later
        RESOURCE_MAP_.insert(std::make_pair(resourceID_, internal_));
    }
}

/* [FUTURE]
    If ever in need to specify different filters / formats
    / attachments, this constructor will need some modification.
    There could be another file somewhere named
    assetName_descriptor.xml that would contain the specific
    information required to build the textures for the asset
    named assetName. If such a file were found, there would
    be a first initialization step with the data extracted
    from the file put into arrays later addressed to the
    TextureInternal constructor via make_shared().
*/
/*
Texture::Texture(const char* asset_name,
                 GLenum filter,
                 GLenum internalFormat,
                 GLenum format,
                 bool clamp,
                 bool lazy_mipmap):
resourceID_(H_(asset_name))
{
    // Find asset files
    AMap::iterator it = ASSET_MAP_.find(resourceID_);
    if(it == ASSET_MAP_.end())
    {
        std::stringstream ss;
        ss << "[Texture] Unable to find asset name: <n>" << asset_name << "</n>";
        DLOGF(ss.str());
        //std::cout << asset_name << std::endl;
        throw std::runtime_error("Unable to find asset by name.");
    }

    // Get file names for all texture file related to asset name
    auto& fileNames = it->second;
    uint32_t numTextures = fileNames.size();

    #if __DEBUG_TEXTURE_VERBOSE__
    {
        std::stringstream ss;
        ss << "[Texture] New texture from asset name: <n>" << asset_name << "</n>";
        DLOGN(ss.str());
    }
    #endif

    // Try to find a cached version first
    auto it2 = RESOURCE_MAP_.find(resourceID_);

    // Push uniform sampler names
    for(uint32_t ii = 0; ii < numTextures; ++ii)
    {
        auto samplerName = extract_texture_sampler_name(fileNames[ii]);
        uniform_sampler_names_.push_back(samplerName);
        #if __DEBUG_TEXTURE_VERBOSE__
            if(it2 == RESOURCE_MAP_.end())
            {
                std::stringstream ss;
                ss << "<v>" << samplerName << "</v> [" << ii << "] <- <p>"
                   << fileNames[ii] << "</p>";
                DLOGI(ss.str());
            }
        #endif
    }

    // If cached version exists, use it.
    if(it2 != RESOURCE_MAP_.end())
    {
        internal_ = it2->second;
#if __DEBUG_TEXTURE_VERBOSE__
        DLOGI("<i>Using cache.</i>");
#endif
        return;
    }

    // Load default textures if needed
    for(const std::string& samplerName : W_MANDATORY_SAMPLERS_)
    {
        if(std::find(uniform_sampler_names_.begin(),
                     uniform_sampler_names_.end(),
                     samplerName) == uniform_sampler_names_.end())
        {
            uniform_sampler_names_.push_back(samplerName);
            std::string path(TEX_IMAGE_PATH);
            path += "default_";
            path += samplerName;
            path += ".png";
            fileNames.push_back(path);
            ++numTextures;
#if __DEBUG_TEXTURE_VERBOSE__
            std::stringstream ss;
            ss << "<v>" << samplerName << "</v> [" << numTextures-1
               << "] <- <d>DEFAULT</d>";
            DLOGI(ss.str());
#endif
        }
    }

    // Did not find a cached version, load files
    unsigned char** data    = new unsigned char*[numTextures];
    PixelBuffer** px_bufs   = new PixelBuffer*[numTextures];
    GLenum* filters         = new GLenum[numTextures];
    GLenum* internalFormats = new GLenum[numTextures];
    GLenum* formats         = new GLenum[numTextures];

    for (uint32_t ii=0; ii<numTextures; ++ii)
    {
        filters[ii]         = filter;
        if(uniform_sampler_names_[ii]==std::string("mt.diffuseTex"))
        {
            // Load Albedo / Diffuse textures as sRGB to avoid
            // double gamma-correction.
            internalFormats[ii] = GL_SRGB_ALPHA;
        }
        else
        {
            internalFormats[ii] = internalFormat;
        }
        formats[ii]         = format;
        try
        {
            px_bufs[ii] = PngLoader::Instance().load_png(fileNames[ii].c_str());
            data[ii] = px_bufs[ii]->get_data_pointer();
#if __DEBUG_TEXTURE_VERBOSE__
            DLOGN("[PixelBuffer] <z>[" + std::to_string(ii) + "]</z>");
            std::cout << *px_bufs[ii] << std::endl;
#endif
        }
        catch(const std::exception& e)
        {
            DLOGF("[Texture] Unable to load Texture.");
            for (uint32_t jj=0; jj<=ii; ++jj)
                if(px_bufs[jj])
                    delete px_bufs[jj];
            delete [] formats;
            delete [] internalFormats;
            delete [] filters;
            delete [] data;
            delete [] px_bufs;
            throw;
        }
    }

    // Create new texture internal
    internal_ = std::make_shared<TextureInternal>(GL_TEXTURE_2D,
                                                  numTextures,
                                                  px_bufs[0]->get_width(),
                                                  px_bufs[0]->get_height(),
                                                  data,
                                                  filters,
                                                  internalFormats,
                                                  formats,
                                                  clamp,
                                                  lazy_mipmap);

    // Free pixel buffers
    for (uint32_t jj=0; jj<numTextures; ++jj)
        if(px_bufs[jj])
            delete px_bufs[jj];
    delete [] formats;
    delete [] internalFormats;
    delete [] filters;
    delete [] data;
    delete [] px_bufs;

    // Cache resource for later
    RESOURCE_MAP_.insert(std::make_pair(resourceID_, internal_));
}*/

Texture::~Texture()
{
    // If only the object which caused destruction of Texture
    // and the resource map hold a reference to texture,
    // no need to cache texture anymore
    if(internal_.use_count() == 2)
    {
        if(resourceID_ != H_(""))
        {
            RESOURCE_MAP_.erase(resourceID_);
            #ifdef __DEBUG_TEXTURE_VERBOSE__
            std::stringstream ss;
            ss << "[Texture] Destroying cached texture: <n>" << resourceID_ << "</n>";
            DLOGN(ss.str());
            #endif
        }
    }
}

void Texture::bind(GLuint unit) const
{
    assert(unit >= 0 && unit <= 31);
    glActiveTexture(GL_TEXTURE0 + unit);
    internal_->bind(unit);

#ifdef __DEBUG_TEXTURE__
    internal_->set_binding_state(unit, unit);
#endif
}

void Texture::bind(GLuint unit, uint32_t index) const
{
    assert(unit >= 0 && unit <= 31);
    assert(index >= 0 && index < internal_->get_num_textures());
    glActiveTexture(GL_TEXTURE0 + unit);
    internal_->bind(index);

#ifdef __DEBUG_TEXTURE__
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
#ifdef __DEBUG_TEXTURE__
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
