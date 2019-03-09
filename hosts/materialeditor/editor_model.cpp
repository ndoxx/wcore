#include <vector>
#include <fstream>
#include <string>
#include <QListView>
#include <QImage>
#include <QSortFilterProxyModel>

#include "editor_model.h"
#include "texlist_model.h"
#include "logger.h"
#include "xml_parser.h"
#include "xml_utils.hpp"

#include "vendor/rapidxml/rapidxml.hpp"
#include "vendor/rapidxml/rapidxml_print.hpp"

using namespace wcore;
using namespace rapidxml;

namespace medit
{

static std::map<int, std::string> texmap_names =
{
    {0, "Albedo"},
    {1, "Roughness"},
    {2, "Metallic"},
    {3, "AO"},
    {4, "Depth"},
    {5, "Normal"}
};

static std::map<hash_t, int> texmap_names_to_index =
{
    {"Albedo"_h, 0},
    {"Roughness"_h, 1},
    {"Metallic"_h, 2},
    {"AO"_h, 3},
    {"Depth"_h, 4},
    {"Normal"_h, 5}
};

TextureMap::TextureMap():
has_image(false),
use_image(false)
{

}

#ifdef __DEBUG__
void TextureMap::debug_display()
{

}
void AlbedoMap::debug_display()
{
    if(has_image) DLOGI("albedo: <p>"    + path.toStdString() + "</p>", "core", Severity::LOW);
}
void RoughnessMap::debug_display()
{
    if(has_image) DLOGI("roughness: <p>" + path.toStdString() + "</p>", "core", Severity::LOW);
}
void MetallicMap::debug_display()
{
    if(has_image) DLOGI("metallic: <p>"  + path.toStdString() + "</p>", "core", Severity::LOW);
}
void AOMap::debug_display()
{
    if(has_image) DLOGI("ao: <p>"        + path.toStdString() + "</p>", "core", Severity::LOW);
}
void DepthMap::debug_display()
{
    if(has_image) DLOGI("depth: <p>"     + path.toStdString() + "</p>", "core", Severity::LOW);
}
void NormalMap::debug_display()
{
    if(has_image) DLOGI("normal: <p>"    + path.toStdString() + "</p>", "core", Severity::LOW);
}
void TextureEntry::debug_display()
{
    DLOG("Texture <n>" + name.toStdString() + "</n>", "core", Severity::LOW);
    for(int ii=0; ii<NTEXMAPS; ++ii)
        texture_maps[ii]->debug_display();
}
#endif

TextureEntry::TextureEntry():
texture_maps({new AlbedoMap,
              new RoughnessMap,
              new MetallicMap,
              new AOMap,
              new DepthMap,
              new NormalMap})
{

}

TextureEntry::TextureEntry(const TextureEntry& other):
texture_maps({new AlbedoMap(*static_cast<AlbedoMap*>(other.texture_maps[0])),
              new RoughnessMap(*static_cast<RoughnessMap*>(other.texture_maps[1])),
              new MetallicMap(*static_cast<MetallicMap*>(other.texture_maps[2])),
              new AOMap(*static_cast<AOMap*>(other.texture_maps[3])),
              new DepthMap(*static_cast<DepthMap*>(other.texture_maps[4])),
              new NormalMap(*static_cast<NormalMap*>(other.texture_maps[5]))}),
name(other.name),
width(other.width),
height(other.height)
{

}

TextureEntry::~TextureEntry()
{
    for(int ii=0; ii<NTEXMAPS; ++ii)
        delete texture_maps[ii];
}

hash_t TextureEntry::parse_node(xml_node<>* mat_node)
{
    // Parse entry name
    std::string entryname;
    xml::parse_attribute(mat_node, "name", entryname);
    xml::parse_attribute(mat_node, "width", width);
    xml::parse_attribute(mat_node, "height", height);
    name = QString::fromStdString(entryname);

    // Get TextureMaps node
    xml_node<>* texmaps_node = mat_node->first_node("TextureMaps");
    if(texmaps_node)
    {
        // For each texturemap
        for(xml_node<>* texmap_node=texmaps_node->first_node("TextureMap");
            texmap_node;
            texmap_node=texmap_node->next_sibling("TextureMap"))
        {
            std::string texmap_name, texmap_path, texmap_unival;
            xml::parse_attribute(texmap_node, "name", texmap_name);
            int index = texmap_names_to_index[H_(texmap_name.c_str())];
            if(xml::parse_attribute(texmap_node, "path", texmap_path))
            {
                texture_maps[index]->has_image = true;
                texture_maps[index]->path = QString::fromStdString(texmap_path);
            }
            if(xml::parse_attribute(texmap_node, "value", texmap_unival))
                texture_maps[index]->parse_uniform_value(texmap_unival);

            // Per-map properties
            xml::parse_node(texmap_node, "TextureMapEnabled", texture_maps[index]->use_image);
        }
    }

    return H_(entryname.c_str());
}

static void node_add_attribute(xml_document<>& doc, xml_node<>* node, const char* attr_name, const char* attr_val)
{
    char* al_attr_name = doc.allocate_string(attr_name);
    char* al_attr_val = doc.allocate_string(attr_val);
    xml_attribute<>* attr = doc.allocate_attribute(al_attr_name, al_attr_val);
    node->append_attribute(attr);
}

static void node_set_value(xml_document<>& doc, xml_node<>* node, const char* value)
{
    node->value(doc.allocate_string(value));
}

void TextureEntry::write_node(rapidxml::xml_document<>& doc, xml_node<>* materials_node)
{
    // Material node with name attribute
    xml_node<>* mat_node = doc.allocate_node(node_element, "Material");
    node_add_attribute(doc, mat_node, "name", name.toUtf8().constData());
    node_add_attribute(doc, mat_node, "width", std::to_string(width).c_str());
    node_add_attribute(doc, mat_node, "height", std::to_string(height).c_str());

    // TextureMaps node to hold texture maps paths
    xml_node<>* texmap_node = doc.allocate_node(node_element, "TextureMaps");
    for(int ii=0; ii<NTEXMAPS; ++ii)
    {
        xml_node<>* tex_node = doc.allocate_node(node_element, "TextureMap");
        node_add_attribute(doc, tex_node, "name", texmap_names[ii].c_str());
        if(texture_maps[ii]->has_image)
            node_add_attribute(doc, tex_node, "path", texture_maps[ii]->path.toUtf8().constData());

        // Save uniform value even if not used
        std::string value_str(texture_maps[ii]->uniform_value_string());
        if(value_str.size()!=0)
            node_add_attribute(doc, tex_node, "value", value_str.c_str());

        // Save per-map properties
        xml_node<>* useimg_node = doc.allocate_node(node_element, "TextureMapEnabled");
        node_set_value(doc, useimg_node, texture_maps[ii]->use_image ? "true" : "false");
        tex_node->append_node(useimg_node);

        texmap_node->append_node(tex_node);
    }

    mat_node->append_node(texmap_node);
    materials_node->append_node(mat_node);
}

void AlbedoMap::parse_uniform_value(const std::string& value_str)
{
    str_val(value_str.c_str(), u_albedo);
}

void RoughnessMap::parse_uniform_value(const std::string& value_str)
{
    str_val(value_str.c_str(), u_roughness);
}

void MetallicMap::parse_uniform_value(const std::string& value_str)
{
    str_val(value_str.c_str(), u_metallic);
}

void AOMap::parse_uniform_value(const std::string& value_str)
{
    str_val(value_str.c_str(), u_ao);
}

void DepthMap::parse_uniform_value(const std::string& value_str)
{

}

void NormalMap::parse_uniform_value(const std::string& value_str)
{

}

std::string AlbedoMap::uniform_value_string()
{
    return wcore::to_string(u_albedo);
}

std::string RoughnessMap::uniform_value_string()
{
    return std::to_string(u_roughness);
}

std::string MetallicMap::uniform_value_string()
{
    return std::to_string(u_metallic);
}

std::string AOMap::uniform_value_string()
{
    return std::to_string(u_ao);
}

std::string DepthMap::uniform_value_string()
{
    return "";
}

std::string NormalMap::uniform_value_string()
{
    return "";
}

EditorModel::EditorModel():
texlist_model_(new TexListModel),
texlist_sort_proxy_model_(new QSortFilterProxyModel)
{

}

EditorModel::~EditorModel()
{
    delete texlist_model_;
    delete texlist_sort_proxy_model_;
}

void EditorModel::set_output_folder(const QString& path)
{
    output_folder_ = QDir(path);
    if(!output_folder_.exists())
    {
        DLOGE("Output texture folder does not exist:", "core", Severity::CRIT);
        DLOGI(path.toStdString(), "core", Severity::CRIT);
    }
}

void EditorModel::set_current_texture_name(const QString& name)
{
    current_texname_ = name;
    DLOGN("Working on texture <n>" + current_texname_.toStdString() + "</n>", "core", Severity::LOW);
}

void EditorModel::setup_list_model(QListView* listview)
{
    // Custom model uses string list as data
    texlist_model_->setStringList(texlist_);
    // Proxy model for sorting the texture list view when a new item is added
    texlist_sort_proxy_model_->setDynamicSortFilter(true);
    texlist_sort_proxy_model_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    texlist_sort_proxy_model_->setFilterKeyColumn(0);
    texlist_sort_proxy_model_->setSourceModel(texlist_model_);
    listview->setModel(texlist_sort_proxy_model_);
}

QModelIndex EditorModel::add_texture(const QString& name)
{
    // Append texture name to list view data and sort
    QModelIndex index = texlist_model_->append(name);
    texlist_sort_proxy_model_->sort(0);

    // Add texture descriptor
    texture_descriptors_.insert(std::pair(H_(name.toUtf8().constData()), TextureEntry()));

    // index is a source index and needs to be remapped to proxy sorted index
    return texlist_sort_proxy_model_->mapFromSource(index);
}

bool EditorModel::has_entry(wcore::hash_t name)
{
    return (texture_descriptors_.find(name) != texture_descriptors_.end());
}

void EditorModel::delete_current_texture(QListView* tex_list)
{
    if(!current_texname_.isEmpty())
    {
        hash_t hname = H_(current_texname_.toUtf8().constData());
        auto it = texture_descriptors_.find(hname);
        if(it != texture_descriptors_.end())
        {
            current_texname_ = "";
            texture_descriptors_.erase(it);
            texlist_model_->removeRow(texlist_sort_proxy_model_->mapToSource(tex_list->currentIndex()).row());
        }
    }
}

void EditorModel::rename_texture(const QString& old_name, const QString& new_name)
{
    // Just remap the descriptor to the new name hash
    hash_t hname = H_(old_name.toUtf8().constData());
    auto it = texture_descriptors_.find(hname);
    if(it != texture_descriptors_.end())
    {
        hash_t new_hname = H_(new_name.toUtf8().constData());
        current_texname_ = new_name;
        texture_descriptors_.insert(std::pair(new_hname, it->second));
        texture_descriptors_.erase(it);
    }
}

void EditorModel::compile(const QString& texname)
{
    DLOGN("Compiling texture <n>" + texname.toStdString() + "</n>", "core", Severity::LOW);

    hash_t hname = H_(texname.toUtf8().constData());
    TextureEntry& entry = get_texture_entry(hname);

    if(entry.width && entry.height)
    {
        DLOGI("Size: WxH= <v>" + std::to_string(entry.width) + "x" + std::to_string(entry.height) + "</v>", "core", Severity::LOW);

        QImage block0(entry.width, entry.height, QImage::Format_RGBA8888);
        QImage block1(entry.width, entry.height, QImage::Format_RGBA8888);
        QImage block2(entry.width, entry.height, QImage::Format_RGBA8888);

        QImage** texmaps = new QImage*[NTEXMAPS];
        for(int ii=0; ii<NTEXMAPS; ++ii)
        {
            if(entry.texture_maps[ii]->has_image)
                texmaps[ii] = new QImage(entry.texture_maps[ii]->path);
            else
                texmaps[ii] = nullptr;
        }

        // Block 0
        for(int xx=0; xx<entry.width; ++xx)
        {
            for(int yy=0; yy<entry.height; ++yy)
            {
                QRgb albedo = texmaps[0] ? texmaps[0]->pixel(xx,yy) : qRgba(0,0,0,1);

                block0.setPixel(xx, yy, albedo);
            }
        }

        // Block 1
        for(int xx=0; xx<entry.width; ++xx)
        {
            for(int yy=0; yy<entry.height; ++yy)
            {
                QRgb normal = texmaps[5] ? texmaps[5]->pixel(xx,yy) : qRgb(0,0,0);
                int depth   = texmaps[4] ? qRed(texmaps[4]->pixel(xx,yy)) : 0;

                QRgb out_color = qRgba(qRed(normal), qGreen(normal), qBlue(normal), depth);
                block1.setPixel(xx, yy, out_color);
            }
        }

        // Block 2
        for(int xx=0; xx<entry.width; ++xx)
        {
            for(int yy=0; yy<entry.height; ++yy)
            {
                int metallic  = texmaps[2] ? qRed(texmaps[2]->pixel(xx,yy)) : 0;
                int ao        = texmaps[3] ? qRed(texmaps[3]->pixel(xx,yy)) : 0;
                int roughness = texmaps[1] ? qRed(texmaps[1]->pixel(xx,yy)) : 0;

                QRgb out_color = qRgba(metallic, ao, roughness, 0);
                block2.setPixel(xx, yy, out_color);
            }
        }

        // Save blocks
        QString block0_name = texname + "_block0.png";
        QString block1_name = texname + "_block1.png";
        QString block2_name = texname + "_block2.png";

        DLOGI("block0: <p>" + block0_name.toStdString() + "</p>", "core", Severity::LOW);
        DLOGI("block1: <p>" + block1_name.toStdString() + "</p>", "core", Severity::LOW);
        DLOGI("block2: <p>" + block2_name.toStdString() + "</p>", "core", Severity::LOW);

        block0.save(output_folder_.filePath(block0_name));
        block1.save(output_folder_.filePath(block1_name));
        block2.save(output_folder_.filePath(block2_name));

        delete[] texmaps;
    }
}

void EditorModel::clear()
{
    current_texname_ = "";
    current_project_ = "";
    texture_descriptors_.clear();
    texlist_ = QStringList{};
    texlist_model_->setStringList(texlist_);
}

void EditorModel::set_project_folder(const QString& path)
{
    project_folder_ = QDir(path);
    if(!project_folder_.exists())
    {
        DLOGE("Project folder does not exist:", "core", Severity::CRIT);
        DLOGI(path.toStdString(), "core", Severity::CRIT);
    }
}

void EditorModel::new_project(const QString& project_name)
{
    // Validate project name and create a new empty project
    if(validate_project_name(project_name))
    {
        // If a project is open, save it and close it
        if(!current_project_.isEmpty())
            close_project();
        // Else, unnamed project became named

        current_project_ = project_name;
        DLOGN("New project: <n>" + project_name.toStdString() + "</n>", "core", Severity::LOW);
    }
}

void EditorModel::open_project(const QString& infile)
{
    DLOGN("Opening project:", "core", Severity::LOW);
    DLOGI("<p>" + infile.toStdString() + "</p>", "core", Severity::LOW);

    // * Open and parse project file
    std::ifstream ifs(infile.toStdString());
    XMLParser parser(ifs);
    xml_node<>* root = parser.get_root(); // MEditProject node

    // Get project name
    std::string project_name;
    xml::parse_attribute(root, "name", project_name);
    current_project_ = QString::fromStdString(project_name);

    xml_node<>* mats_node = root->first_node("Materials");

    // * Retrieve descriptors and initialize model
    for(xml_node<>* mat_node=mats_node->first_node("Material");
        mat_node;
        mat_node=mat_node->next_sibling("Material"))
    {

        TextureEntry entry;
        hash_t hentryname = entry.parse_node(mat_node);
        // Insert descriptor
        texture_descriptors_.insert(std::pair(hentryname, entry));
        // Populate texture list
        texlist_model_->append(entry.name);
    }
    texlist_sort_proxy_model_->sort(0);
}

void EditorModel::save_project_as(const QString& project_name)
{
    if(!project_name.isEmpty())
    {
        DLOGN("Saving project: <n>" + project_name.toStdString() + "</n>", "core", Severity::LOW);
        QString filepath = project_path_from_name(project_name);

        // * Generate XML output from descriptors and write to file
        // Doctype declaration
        xml_document<> doc;
        xml_node<>* decl = doc.allocate_node(node_declaration);
        decl->append_attribute(doc.allocate_attribute("version", "1.0"));
        decl->append_attribute(doc.allocate_attribute("encoding", "UTF-8"));
        doc.append_node(decl);

        // Root node
        xml_node<>* root = doc.allocate_node(node_element, "MEditProject");
        doc.append_node(root);

        // Project name
        node_add_attribute(doc, root, "name", project_name.toUtf8().constData());

        // Config node
        //xml_node<>* cfg_node = doc.allocate_node(node_element, "Config");

        // Materials node
        xml_node<>* mats_node = doc.allocate_node(node_element, "Materials");

        // Write descriptors
        for(auto&& [key, entry]: texture_descriptors_)
            entry.write_node(doc, mats_node);

        root->append_node(mats_node);

        std::ofstream outfile;
        outfile.open(filepath.toUtf8().constData());
        outfile << doc;

        //std::cout << doc << std::endl;

        current_project_ = project_name;
    }
}

void EditorModel::close_project()
{
    save_project();
    DLOGN("Closing project: <n>" + current_project_.toStdString() + "</n>", "core", Severity::LOW);
    clear();
}

bool EditorModel::save_project()
{
    if(!current_project_.isEmpty())
    {
        save_project_as(current_project_);
        return true;
    }
    return false;
}

bool EditorModel::validate_project_name(const QString& name)
{
    if(name.isEmpty()) return false;

    // Check that name is alphanumeric (underscores are allowed)
    QRegularExpression re("^[a-zA-Z0-9_]+$");
    QRegularExpressionMatch match = re.match(name);

    bool ret = match.hasMatch();

    if(!ret)
    {
        DLOGW("Invalid project name.", "core", Severity::WARN);
        DLOGI("<h>Rules</h>: alphanumeric, no space, underscores are allowed.", "core", Severity::WARN);
    }

    return ret;
}

QString EditorModel::project_path_from_name(const QString& name)
{
    return project_folder_.filePath(project_file_from_name(name));
}

QString EditorModel::project_file_from_name(const QString& name)
{
    return name + ".wmp";
}


} // namespace medit
