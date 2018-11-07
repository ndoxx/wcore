#include <cstring>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <functional>
#include <vector>
#include "external/rapidxml.hpp"

#include "math3d.h"

using namespace rapidxml;

template <typename T>
bool str_val(const char* value, T& result)
{
    std::istringstream iss(value);
    return !(iss >> result).fail();
}

template <>
bool str_val<math::vec<3> >(const char* value, math::vec<3>& result)
{
    return sscanf(value, "(%f,%f,%f)", &result[0], &result[1], &result[2]) > 0;
}

template <>
bool str_val<bool>(const char* value, bool& result)
{
    result = !strcmp(value, "true");
    return true;
}

template <typename T>
void parse_attribute(xml_node<>* node, const char* name, std::function<void(T value)> exec)
{
    T value;
    bool success = str_val(node->first_attribute(name)->value(), value);
    if(success)
        exec(value);
}

template <typename T>
void parse_leaf(xml_node<>* node, const char* name, std::function<void(T value)> exec)
{
    xml_node<>* leaf_node = node->first_node(name);
    if(!leaf_node)
        return;

    T value;
    bool success = str_val(leaf_node->value(), value);
    if(success)
        exec(value);
}

template <>
void parse_leaf<const char*>(xml_node<>* node, const char* name, std::function<void(const char* value)> exec)
{
    xml_node<>* leaf_node = node->first_node(name);
    if(!leaf_node)
        return;

    exec(leaf_node->value());
}

int main(void)
{
    std::cout << "Parsing scene description..." << std::endl;
    // Read the xml file into a vector
    std::ifstream scene_file("../res/xml/crystal_scene.xml");
    std::vector<char> buffer((std::istreambuf_iterator<char>(scene_file)), std::istreambuf_iterator<char>());
    buffer.push_back('\0');
    // Parse the buffer using the xml file parsing library into doc
    xml_document<> doc;
    doc.parse<0>(&buffer[0]);
    // Find our root node
    xml_node<>* root = doc.first_node("Scene");

    // Iterate over terrains
    xml_node<>* terr = root->first_node("Terrain");
    if(terr)
    {
        for (xml_node<>* patch=terr->first_node("TerrainPatch"); patch; patch=patch->next_sibling())
        {
            std::cout << "Terrain patch: " << std::endl;

            parse_attribute<uint32_t>(patch, "width", [&](uint32_t width)
            {
                std::cout << ".width= " << width << std::endl;
            });
            parse_attribute<uint32_t>(patch, "length", [&](uint32_t length)
            {
                std::cout << ".length= " << length << std::endl;
            });
            parse_attribute<uint32_t>(patch, "height", [&](uint32_t height)
            {
                std::cout << ".height= " << height << std::endl;
            });

            parse_leaf<math::vec3>(patch, "Position", [&](const math::vec3& position)
            {
                std::cout << "\t-> Position= " << position << std::endl;
            });
            /*parse_leaf<float>(patch, "Schmoult", [&](float position)
            {
                std::cout << "\t-> Schmoult= " << position << std::endl;
            });*/

            xml_node<>* mat_node = patch->first_node("Material");
            if(mat_node)
            {
                std::cout << "\t-> Material: " << std::endl;
                parse_leaf<const char*>(mat_node, "Asset", [&](const char* asset)
                {
                    std::cout << "\t\t-> Asset= " << asset << std::endl;
                });
                parse_leaf<bool>(mat_node, "NormalMap", [&](bool use_normal_map)
                {
                    std::cout << "\t\t-> use_normal_map= " << use_normal_map << std::endl;
                });
                parse_leaf<float>(mat_node, "ParallaxHeightScale", [&](float parallax_height_scale)
                {
                    std::cout << "\t\t-> parallax_height_scale= " << parallax_height_scale << std::endl;
                });
            }

            xml_node<>* aabb_node = patch->first_node("AABB");
            if(aabb_node)
            {
                std::cout << "\t-> AABB: " << std::endl;
                parse_leaf<math::vec3>(aabb_node, "Offset", [&](const math::vec3& offset)
                {
                    std::cout << "\t\t-> offset= " << offset << std::endl;
                });
            }

            xml_node<>* shadow_node = patch->first_node("Shadow");
            if(shadow_node)
            {
                std::cout << "\t-> Shadow: " << std::endl;
                parse_leaf<uint32_t>(shadow_node, "CullFace", [&](uint32_t cull_face)
                {
                    std::cout << "\t\t-> cull_face= " << cull_face << std::endl;
                });
            }

            std::cout << std::endl;
        }
    }


    // Iterate over models
    xml_node<>* mdls = root->first_node("Models");
    if(mdls)
    {
        for (xml_node<>* model=mdls->first_node("Model"); model; model=model->next_sibling())
        {
            std::cout << "Model: " << std::endl;
            parse_attribute<uint32_t>(model, "id", [&](uint32_t id)
            {
                std::cout << ".id= " << id << std::endl;
            });
            parse_leaf<const char*>(model, "Mesh", [&](const char* mesh)
            {
                std::cout << "\t-> Mesh= " << mesh << std::endl;
            });


            xml_node<>* mat_node = model->first_node("Material");
            if(mat_node)
            {
                std::cout << "\t-> Material: " << std::endl;
                parse_leaf<const char*>(mat_node, "Asset", [&](const char* asset)
                {
                    std::cout << "\t\t-> Asset= " << asset << std::endl;
                });
                parse_leaf<bool>(mat_node, "NormalMap", [&](bool use_normal_map)
                {
                    std::cout << "\t\t-> use_normal_map= " << use_normal_map << std::endl;
                });
                parse_leaf<float>(mat_node, "ParallaxHeightScale", [&](float parallax_height_scale)
                {
                    std::cout << "\t\t-> parallax_height_scale= " << parallax_height_scale << std::endl;
                });
                parse_leaf<bool>(mat_node, "Overlay", [&](bool overlay)
                {
                    std::cout << "\t\t-> overlay= " << overlay << std::endl;
                });
                parse_leaf<math::vec3>(mat_node, "Color", [&](const math::vec3& color)
                {
                    std::cout << "\t\t-> color= " << color << std::endl;
                });
                parse_leaf<float>(mat_node, "Roughness", [&](float roughness)
                {
                    std::cout << "\t\t-> roughness= " << roughness << std::endl;
                });
            }

            std::cout << std::endl;
        }
    }
}
