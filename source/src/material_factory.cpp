#include <GL/glew.h>

#include "material_factory.h"
#include "logger.h"

using namespace rapidxml;

TextureParameters::TextureParameters():
filter(GL_LINEAR_MIPMAP_LINEAR),
internal_format(GL_RGBA),
format(GL_RGBA),
clamp(false),
lazy_mipmap(false)
{

}


MaterialFactory::MaterialFactory(const char* filename)
{
    xml_parser_.load_file_xml(filename);
    retrieve_asset_descriptions(xml_parser_.get_root());
}

MaterialFactory::~MaterialFactory()
{

}

#ifdef __DEBUG__
std::ostream& operator<< (std::ostream& stream, const MaterialDescriptor& desc)
{
    TextureDescriptor::TexMap::const_iterator it;

    stream << "Albedo: ";
    if((it = desc.texture_descriptor.locations.find(HS_("Albedo"))) != desc.texture_descriptor.locations.end())
        stream << it->second << std::endl;
    else
        stream <<  desc.albedo << std::endl;

    stream << "Metallic: ";
    if((it = desc.texture_descriptor.locations.find(HS_("Metallic"))) != desc.texture_descriptor.locations.end())
        stream << it->second << std::endl;
    else
        stream <<  desc.metallic << std::endl;

    stream << "Roughness: ";
    if((it = desc.texture_descriptor.locations.find(HS_("Roughness"))) != desc.texture_descriptor.locations.end())
        stream << it->second << std::endl;
    else
        stream <<  desc.roughness << std::endl;

    if((it = desc.texture_descriptor.locations.find(HS_("AO"))) != desc.texture_descriptor.locations.end())
    {
        stream << "Ambient Occlusion: " << it->second << std::endl;
    }

    if((it = desc.texture_descriptor.locations.find(HS_("Normal"))) != desc.texture_descriptor.locations.end())
    {
        stream << "Normal map: " << it->second << std::endl;
        if(!desc.enable_normal_mapping)
            stream << "Normal map disabled." << std::endl;
    }

    if((it = desc.texture_descriptor.locations.find(HS_("Depth"))) != desc.texture_descriptor.locations.end())
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

void MaterialFactory::retrieve_asset_descriptions(rapidxml::xml_node<>* root)
{
    for (xml_node<>* mat_node=xml_parser_.get_root()->first_node("Material");
         mat_node;
         mat_node=mat_node->next_sibling("Material"))
    {
        std::string material_name;
        if(xml::parse_node(mat_node, "Name", material_name))
        {
#ifdef __DEBUG__
            if(material_descriptors_.find(H_(material_name.c_str())) != material_descriptors_.end())
            {
                DLOGW("[MaterialFactory] Material redefinition or collision: ");
                DLOGI(material_name);
            }
#endif

            MaterialDescriptor descriptor;
            parse_material_descriptor(mat_node, descriptor);
            descriptor.texture_descriptor.resource_id = H_(material_name.c_str());
            material_descriptors_[H_(material_name.c_str())] = descriptor;
        }
    }
}

static std::map<hashstr_t, const char*> TEX_UNITS =
{
    {HS_("Albedo"),    "Albedo"},
    {HS_("AO"),        "AO"},
    {HS_("Depth"),     "Depth"},
    {HS_("Metallic"),  "Metallic"},
    {HS_("Normal"),    "Normal"},
    {HS_("Roughness"), "Roughness"}
};

void MaterialFactory::parse_material_descriptor(rapidxml::xml_node<>* node, MaterialDescriptor& descriptor)
{
    // Register texture units
    xml_node<>* tex_node = node->first_node("Texture");
    if(tex_node)
    {
        std::string texture_map;
        for(auto&& [key, node_name]: TEX_UNITS)
        {
            if((descriptor.texture_descriptor.has_unit[key] = xml::parse_node(tex_node, node_name, texture_map)))
            {
                descriptor.texture_descriptor.locations[key] = texture_map;
                //descriptor.texture_descriptor.parameters[key] = TextureParameters();
            }
        }
    }

    // Uniform alternatives
    xml_node<>* uni_node = node->first_node("Uniforms");
    if(uni_node)
    {
        if(!descriptor.texture_descriptor.has_unit[HS_("Albedo")])
            xml::parse_node(uni_node, "Albedo", descriptor.albedo);
        if(!descriptor.texture_descriptor.has_unit[HS_("Metallic")])
            xml::parse_node(uni_node, "Metallic", descriptor.metallic);
        if(!descriptor.texture_descriptor.has_unit[HS_("Roughness")])
            xml::parse_node(uni_node, "Roughness", descriptor.roughness);

        descriptor.has_transparency = xml::parse_node(uni_node, "Transparency", descriptor.transparency);
    }

    // Override
    if(descriptor.texture_descriptor.has_unit[HS_("Normal")])
        xml::parse_node(node, "NormalMap", descriptor.enable_normal_mapping);
    if(descriptor.texture_descriptor.has_unit[HS_("Depth")])
        xml::parse_node(node, "ParallaxMap", descriptor.enable_parallax_mapping);

    // Shading options
    xml::parse_node(node, "ParallaxHeightScale", descriptor.parallax_height_scale);
}
