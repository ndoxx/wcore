#include "material_factory.h"
#include "material.h"
#include "texture.h"
#include "cubemap.h"
#include "wat_loader.h"
#include "colors.h"
#include "logger.h"
#include "file_system.h"
#include "error.h"

namespace wcore
{

using namespace rapidxml;

std::map<TextureUnit, const char*> MaterialFactory::TEX_SAMPLERS_NODES =
{
    {TextureUnit::BLOCK0, "Block0"},
    {TextureUnit::BLOCK1, "Block1"},
    {TextureUnit::BLOCK2, "Block2"}
};

MaterialFactory::MaterialFactory(const char* xml_file):
wat_loader_(new WatLoader())
{
    auto pstream = FILESYSTEM.get_file_as_stream(xml_file, "root.folders.level"_h, "pack0"_h);
    if(pstream == nullptr)
    {
        DLOGE("[MaterialFactory] Unable to open file:", "material");
        DLOGI("<p>" + std::string(xml_file) + "</p>", "material");
        fatal();
    }
    xml_parser_.load_file_xml(*pstream);
    retrieve_material_descriptions(xml_parser_.get_root()->first_node("Materials"));
}

MaterialFactory::MaterialFactory():
wat_loader_(new WatLoader())
{

}

MaterialFactory::~MaterialFactory()
{
    delete wat_loader_;
    /*for(auto&& [key, mat_ptr]: cache_)
        delete mat_ptr;*/
}

#ifdef __DEBUG__
std::ostream& operator<< (std::ostream& stream, const MaterialDescriptor& desc)
{
    TextureDescriptor::TexMap::const_iterator it;

    stream << "Albedo: ";
    if((it = desc.texture_descriptor.locations.find(TextureUnit::ALBEDO)) != desc.texture_descriptor.locations.end())
        stream << it->second << std::endl;
    else
        stream <<  desc.albedo << std::endl;

    stream << "Metallic: ";
    if((it = desc.texture_descriptor.locations.find(TextureUnit::METALLIC)) != desc.texture_descriptor.locations.end())
        stream << it->second << std::endl;
    else
        stream <<  desc.metallic << std::endl;

    stream << "Roughness: ";
    if((it = desc.texture_descriptor.locations.find(TextureUnit::ROUGHNESS)) != desc.texture_descriptor.locations.end())
        stream << it->second << std::endl;
    else
        stream <<  desc.roughness << std::endl;

    if((it = desc.texture_descriptor.locations.find(TextureUnit::AO)) != desc.texture_descriptor.locations.end())
    {
        stream << "Ambient Occlusion: " << it->second << std::endl;
    }

    if((it = desc.texture_descriptor.locations.find(TextureUnit::NORMAL)) != desc.texture_descriptor.locations.end())
    {
        stream << "Normal map: " << it->second << std::endl;
        if(!desc.enable_normal_mapping)
            stream << "Normal map disabled." << std::endl;
    }

    if((it = desc.texture_descriptor.locations.find(TextureUnit::DEPTH)) != desc.texture_descriptor.locations.end())
    {
        stream << "Depth: " << it->second << " & Parallax height scale: " << desc.parallax_height_scale << std::endl;
        if(!desc.enable_parallax_mapping)
            stream << "Parallax map disabled." << std::endl;
    }

    if(desc.has_transparency)
        stream << "Transparency: " << desc.transparency << std::endl;


    return stream;
}
#endif

void MaterialFactory::retrieve_material_descriptions(rapidxml::xml_node<>* materials_node)
{
    for (xml_node<>* mat_node=materials_node->first_node("Material");
         mat_node;
         mat_node=mat_node->next_sibling("Material"))
    {
        std::string material_name;
        std::string wat_location;
        xml::parse_attribute(mat_node, "name", material_name);
        if(xml::parse_attribute(mat_node, "location", wat_location))
            material_name = wat_location;

        if(material_name.size())
        {
#ifdef __DEBUG__
            if(material_descriptors_.find(H_(material_name.c_str())) != material_descriptors_.end())
            {
                DLOGW("[MaterialFactory] Material redefinition or collision: ", "material");
                DLOGI(material_name, "material");
            }
#endif

            MaterialDescriptor descriptor;
            parse_material_descriptor(mat_node, descriptor);
            descriptor.texture_descriptor.resource_id = H_(material_name.c_str());
            material_descriptors_[H_(material_name.c_str())] = descriptor;
#ifdef __DEBUG__
            HRESOLVE.add_intern_string(material_name);
#endif
        }
    }
}

void MaterialFactory::retrieve_cubemap_descriptions(rapidxml::xml_node<>* cubemaps_node)
{
    for (xml_node<>* cmap_node=cubemaps_node->first_node("CubemapTexture");
         cmap_node;
         cmap_node=cmap_node->next_sibling("CubemapTexture"))
    {
        std::string cubemap_name;
        if(xml::parse_attribute(cmap_node, "name", cubemap_name))
        {
            hash_t resource_id = H_(cubemap_name.c_str());
#ifdef __DEBUG__
            if(cubemap_descriptors_.find(resource_id) != cubemap_descriptors_.end())
            {
                DLOGW("[MaterialFactory] Cubemap redefinition or collision: ", "material");
                DLOGI(cubemap_name, "material");
            }
#endif

            CubemapDescriptor descriptor;
            parse_cubemap_descriptor(cmap_node, descriptor);
            descriptor.resource_id = resource_id;
            cubemap_descriptors_[H_(cubemap_name.c_str())] = descriptor;
#ifdef __DEBUG__
            HRESOLVE.add_intern_string(cubemap_name);
#endif
        }
    }
}

Material* MaterialFactory::make_material(hash_t asset_name, uint8_t sampler_group)
{
    MaterialDescriptor descriptor(get_material_descriptor(asset_name));
    descriptor.texture_descriptor.sampler_group = sampler_group;

    Material* ret = new Material(descriptor);
    return ret;
}

Material* MaterialFactory::make_material(MaterialDescriptor& descriptor)
{
    // TMP
    if(!descriptor.is_wat)
        return new Material(descriptor);
    else
        return nullptr;
}

Material* MaterialFactory::make_material(rapidxml::xml_node<>* material_node,
                                         uint8_t sampler_group,
                                         OptRngT opt_rng)
{
    std::string asset;
    bool use_asset = xml::parse_attribute(material_node, "name", asset);

    if(use_asset)
    {
        return make_material(H_(asset.c_str()), sampler_group);
    }

    MaterialDescriptor desc;
    parse_material_descriptor(material_node, desc, opt_rng);
    return make_material(desc);
}

std::vector<std::string> split(const std::string& s, char delimiter)
{
   std::vector<std::string> tokens;
   std::string token;
   std::istringstream tokenStream(s);
   while(std::getline(tokenStream, token, delimiter))
   {
      tokens.push_back(token);
   }
   return tokens;
}

void MaterialFactory::parse_material_descriptor(rapidxml::xml_node<>* node,
                                                MaterialDescriptor& descriptor,
                                                OptRngT opt_rng)
{
    // Wat format
    std::string wat_location;
    bool is_wat = xml::parse_attribute(node, "location", wat_location);
    if(is_wat)
    {
        descriptor.is_wat = true;
        descriptor.wat_location = wat_location;
    }
    else
    {
        xml_node<>* tex_node = node->first_node("Texture");
        if(tex_node)
        {
            // Register texture units
            std::string units_str;
            if(xml::parse_node(tex_node, "Units", units_str))
            {
                // Units string is a semi-column separated list
                std::vector<std::string> units(split(units_str, ';'));
                for(int ii=0; ii<units.size(); ++ii)
                {
                    switch(H_(units[ii].c_str()))
                    {
                        case "Albedo"_h:
                            descriptor.texture_descriptor.add_unit(TextureUnit::ALBEDO);
                            break;
                        case "Normal"_h:
                            descriptor.texture_descriptor.add_unit(TextureUnit::NORMAL);
                            break;
                        case "Depth"_h:
                            descriptor.texture_descriptor.add_unit(TextureUnit::DEPTH);
                            break;
                        case "Metallic"_h:
                            descriptor.texture_descriptor.add_unit(TextureUnit::METALLIC);
                            break;
                        case "AO"_h:
                            descriptor.texture_descriptor.add_unit(TextureUnit::AO);
                            break;
                        case "Roughness"_h:
                            descriptor.texture_descriptor.add_unit(TextureUnit::ROUGHNESS);
                            break;
                    }
                }
            }
            // Register texture block locations
            std::string texture_map;
            for(auto&& [unit, node_name]: TEX_SAMPLERS_NODES)
            {
                if(xml::parse_node(tex_node, node_name, texture_map))
                {
                    descriptor.texture_descriptor.locations[unit] = texture_map;
                    descriptor.texture_descriptor.add_unit(unit);
                    descriptor.is_textured = true;
                }
            }
        }

        // Uniform alternatives
        xml_node<>* uni_node = node->first_node("Uniform");
        if(uni_node)
        {
            if(!descriptor.texture_descriptor.has_unit(TextureUnit::ALBEDO))
            {
                // Get color space, to convert later to RGB if needed
                std::string color_space;
                xml::parse_attribute(uni_node->first_node("Albedo"), "space", color_space);
                // Get color
                xml::parse_node(uni_node, "Albedo", descriptor.albedo);

                // Check if an rng is in use
                if(opt_rng)
                {
                    std::mt19937& rng = *opt_rng;
                    std::uniform_real_distribution<float> var_distrib(-1.0f,1.0f);
                    math::vec3 color_var(0);
                    xml::parse_attribute(uni_node->first_node("Albedo"), "variance", color_var);
                    descriptor.albedo += math::vec3(color_var.x() * var_distrib(rng),
                                                    color_var.y() * var_distrib(rng),
                                                    color_var.z() * var_distrib(rng));
                }

                if(!color_space.compare("hsl"))
                    descriptor.albedo = color::hsl2rgb(descriptor.albedo);
            }
            if(!descriptor.texture_descriptor.has_unit(TextureUnit::METALLIC))
                xml::parse_node(uni_node, "Metallic", descriptor.metallic);
            if(!descriptor.texture_descriptor.has_unit(TextureUnit::ROUGHNESS))
                xml::parse_node(uni_node, "Roughness", descriptor.roughness);

            descriptor.has_transparency = xml::parse_node(uni_node, "Transparency", descriptor.transparency);

            // Shading options
            xml::parse_node(uni_node, "ParallaxHeightScale", descriptor.parallax_height_scale);
        }
    }

    // Override
    xml_node<>* ovd_node = node->first_node("Override");
    if(ovd_node)
    {
        if(descriptor.texture_descriptor.has_unit(TextureUnit::NORMAL))
            xml::parse_node(ovd_node, "NormalMap", descriptor.enable_normal_mapping);
        if(descriptor.texture_descriptor.has_unit(TextureUnit::DEPTH))
            xml::parse_node(ovd_node, "ParallaxMap", descriptor.enable_parallax_mapping);
    }
}

static std::vector<std::string> CubeMapFaces =
{
    "XPlus", "XMinus", "YPlus", "YMinus", "ZPlus", "ZMinus"
};

void MaterialFactory::parse_cubemap_descriptor(rapidxml::xml_node<>* node,
                                               CubemapDescriptor& descriptor)
{
    // Get nodes for each cubemap face in the right order (-x, +x, -y, +y, -z, +z)
    for(int ii=0; ii<CubeMapFaces.size(); ++ii)
    {
        std::string tex_name;
        if(!xml::parse_node(node, CubeMapFaces[ii].c_str(), tex_name))
        {
            DLOGW("[MaterialFactory] Cubemap texture ill-declared:", "material");
            DLOGI("Missing node <x>" + CubeMapFaces[ii] + "</x>", "material");
            DLOGI("Descriptor will be incomplete.", "material");
            return;
        }
        descriptor.locations.push_back(tex_name);
    }
}

Cubemap* MaterialFactory::make_cubemap(hash_t cubemap_name)
{
    return new Cubemap(get_cubemap_descriptor(cubemap_name));
}

Texture* MaterialFactory::make_texture(std::istream& stream)
{
    return new Texture(stream);
}


}
