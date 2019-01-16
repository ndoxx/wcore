#include "material_factory.h"
#include "material.h"
#include "colors.h"
#include "io_utils.h"
#include "logger.h"

namespace wcore
{

using namespace rapidxml;

std::map<TextureUnit, const char*> MaterialFactory::TEX_SAMPLERS_NODES =
{
    {TextureUnit::ALBEDO,    "Albedo"},
    {TextureUnit::AO,        "AO"},
    {TextureUnit::DEPTH,     "Depth"},
    {TextureUnit::METALLIC,  "Metallic"},
    {TextureUnit::NORMAL,    "Normal"},
    {TextureUnit::ROUGHNESS, "Roughness"}
};

MaterialFactory::MaterialFactory(const char* xml_file)
{
    fs::path file_path(io::get_file(H_("root.folders.level"), xml_file));
    xml_parser_.load_file_xml(file_path);
    retrieve_asset_descriptions(xml_parser_.get_root()->first_node("Materials"));
}

MaterialFactory::MaterialFactory()
{

}

MaterialFactory::~MaterialFactory()
{
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

void MaterialFactory::retrieve_asset_descriptions(rapidxml::xml_node<>* materials_node)
{
    for (xml_node<>* mat_node=materials_node->first_node("Material");
         mat_node;
         mat_node=mat_node->next_sibling("Material"))
    {
        std::string material_name;
        if(xml::parse_attribute(mat_node, "name", material_name))
        {
#ifdef __DEBUG__
            if(material_descriptors_.find(H_(material_name.c_str())) != material_descriptors_.end())
            {
                DLOGW("[MaterialFactory] Material redefinition or collision: ", "material", Severity::WARN);
                DLOGI(material_name, "material", Severity::WARN);
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

Material* MaterialFactory::make_material(hash_t asset_name)
{
    // Try to find in cache, make new material if not cached
    /*auto it = cache_.find(asset_name);
    if(it != cache_.end())
        return it->second;
    else
    {*/
        Material* ret = new Material(get_descriptor(asset_name));
        //cache_.insert(std::pair(asset_name, ret));
        ret->set_cached(true);
        return ret;
    //}
}

Material* MaterialFactory::make_material(MaterialDescriptor& descriptor)
{
    return new Material(descriptor);
}

Material* MaterialFactory::make_material(rapidxml::xml_node<>* material_node, OptRngT opt_rng)
{
    std::string asset;
    bool use_asset = xml::parse_node(material_node, "Asset", asset);

    if(use_asset)
        return make_material(H_(asset.c_str()));

    MaterialDescriptor desc;
    parse_material_descriptor(material_node, desc, opt_rng);
    return make_material(desc);
}


void MaterialFactory::parse_material_descriptor(rapidxml::xml_node<>* node,
                                                MaterialDescriptor& descriptor,
                                                OptRngT opt_rng)
{
    // Register texture units
    xml_node<>* tex_node = node->first_node("Texture");
    if(tex_node)
    {
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
        xml::parse_node(node, "ParallaxHeightScale", descriptor.parallax_height_scale);
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

}
