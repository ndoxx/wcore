#include <stack>
#include <fstream>

#include "xml_skeleton_exporter.h"

#include "vendor/rapidxml/rapidxml_print.hpp"
#include "xml_utils.hpp"
#include "config.h"
#include "logger.h"

using namespace rapidxml;
using namespace wcore;

namespace wconvert
{

static void xml_node_add_attribute(xml_document<>& doc, xml_node<>* node, const char* attr_name, const char* attr_val)
{
    char* al_attr_name = doc.allocate_string(attr_name);
    char* al_attr_val = doc.allocate_string(attr_val);
    xml_attribute<>* attr = doc.allocate_attribute(al_attr_name, al_attr_val);
    node->append_attribute(attr);
}

static void xml_node_set_value(xml_document<>& doc, xml_node<>* node, const char* value)
{
    node->value(doc.allocate_string(value));
}

XMLSkeletonExporter::XMLSkeletonExporter()
{
    wcore::CONFIG.get("root.folders.model"_h, exportdir_);
}

XMLSkeletonExporter::~XMLSkeletonExporter()
{

}

static std::string mat4_to_string(const math::mat4& matrix)
{
    std::string matrix_str;
    for(int ii=0; ii<16; ++ii)
    {
        matrix_str += std::to_string(matrix[ii]);
        if(ii<15)
            matrix_str += " ";
    }
    return matrix_str;
}

static void make_skeletton_DOM(xml_document<>& doc, rapidxml::xml_node<>* parent_xml_node, const Tree<BoneInfo>::nodeT* bone_node)
{
    xml_node<>* current_xml_node = doc.allocate_node(node_element, "bone");
    parent_xml_node->append_node(current_xml_node);

    // Bone name
    xml_node_add_attribute(doc, current_xml_node, "name", bone_node->data.name.c_str());

    // Bone offset matrix
    const math::mat4& offset_matrix = bone_node->data.offset_matrix;
    std::string matrix_str(mat4_to_string(offset_matrix));
    xml_node<>* matrix_xml_node = doc.allocate_node(node_element, "offset");
    current_xml_node->append_node(matrix_xml_node);
    xml_node_set_value(doc, matrix_xml_node, matrix_str.c_str());

    for(auto* child = bone_node->first_node(); child; child = child->next_sibling())
    {
        make_skeletton_DOM(doc, current_xml_node, child);
    }
}

bool XMLSkeletonExporter::export_skeleton(const ModelInfo& model_info)
{
    std::string filename(model_info.model_name + ".skel");

    DLOGN("<i>Exporting</i> skeletton to:", "wconvert");
    DLOGI("<p>" + filename + "</p>", "wconvert");

    // * Produce XML representation of model data
    // Doctype declaration
    xml_document<> doc;
    xml_node<>* decl = doc.allocate_node(node_declaration);
    decl->append_attribute(doc.allocate_attribute("version", "1.0"));
    decl->append_attribute(doc.allocate_attribute("encoding", "UTF-8"));
    doc.append_node(decl);

    // Root node
    xml_node<>* root = doc.allocate_node(node_element, "BoneHierarchy");
    doc.append_node(root);
    xml_node_add_attribute(doc, root, "name", model_info.model_name.c_str());

    // Root transform
    std::string matrix_str(mat4_to_string(model_info.root_transform));
    xml_node<>* root_transform_xml_node = doc.allocate_node(node_element, "offset");
    root->append_node(root_transform_xml_node);
    xml_node_set_value(doc, root_transform_xml_node, matrix_str.c_str());

    // Depth-first traversal of bone hierarchy
    // We want to conserve the hierarchy in XML format
    make_skeletton_DOM(doc, root, model_info.bone_hierarchy.get_root());

    std::ofstream outfile;
    outfile.open(exportdir_ / filename);
    outfile << doc;

    return true;
}


} // namespace wconvert
