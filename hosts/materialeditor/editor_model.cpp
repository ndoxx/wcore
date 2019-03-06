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

TextureEntry::TextureEntry():
has_map({false,false,false,false,false,false})
{

}

void TextureEntry::debug_display()
{
    DLOG("Texture <n>" + name.toStdString() + "</n>", "core", Severity::LOW);
    if(has_map[0]) DLOGI("albedo: <p>"    + paths[0].toStdString() + "</p>", "core", Severity::LOW);
    if(has_map[1]) DLOGI("roughness: <p>" + paths[1].toStdString() + "</p>", "core", Severity::LOW);
    if(has_map[2]) DLOGI("metallic: <p>"  + paths[2].toStdString() + "</p>", "core", Severity::LOW);
    if(has_map[3]) DLOGI("ao: <p>"        + paths[3].toStdString() + "</p>", "core", Severity::LOW);
    if(has_map[4]) DLOGI("depth: <p>"     + paths[4].toStdString() + "</p>", "core", Severity::LOW);
    if(has_map[5]) DLOGI("normal: <p>"    + paths[5].toStdString() + "</p>", "core", Severity::LOW);
}

EditorModel::EditorModel():
texlist_model_(new TexListModel),
texlist_sort_proxy_model_(new QSortFilterProxyModel),
needs_saving_(false)
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
    project_save_requested(true);

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
            project_save_requested(true);
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
        project_save_requested(true);
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
            if(entry.has_map[ii])
                texmaps[ii] = new QImage(entry.paths[ii]);
            else
                texmaps[ii] = nullptr;
        }

        // Block 0
        for(int xx=0; xx<entry.width; ++xx)
        {
            for(int yy=0; yy<entry.width; ++yy)
            {
                QRgb albedo   = texmaps[0] ? texmaps[0]->pixel(xx,yy) : qRgb(0,0,0);
                int roughness = texmaps[1] ? qRed(texmaps[1]->pixel(xx,yy)) : 0;

                QRgb out_color = qRgba(qRed(albedo), qGreen(albedo), qBlue(albedo), roughness);
                block0.setPixel(xx, yy, out_color);
            }
        }

        // Block 1
        for(int xx=0; xx<entry.width; ++xx)
        {
            for(int yy=0; yy<entry.width; ++yy)
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
            for(int yy=0; yy<entry.width; ++yy)
            {
                int metallic = texmaps[2] ? qRed(texmaps[2]->pixel(xx,yy)) : 0;
                int ao       = texmaps[3] ? qRed(texmaps[3]->pixel(xx,yy)) : 0;

                QRgb out_color = qRgba(metallic, ao, 0, 0);
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

void EditorModel::project_save_requested(bool state)
{
    needs_saving_ = state;
    sig_save_requested_state(state);
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
        {
            close_project();
        }
        // Else, unnamed project became named

        current_project_ = project_name;
        DLOGN("New project: <n>" + project_name.toStdString() + "</n>", "core", Severity::LOW);
        project_save_requested(true);
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

        // Parse entry name
        std::string entryname;
        xml::parse_attribute(mat_node, "name", entryname);
        xml::parse_attribute(mat_node, "width", entry.width);
        xml::parse_attribute(mat_node, "height", entry.height);
        hash_t hentryname = H_(entryname.c_str());
        entry.name = QString::fromStdString(entryname);

        // Get TextureMaps node
        xml_node<>* texmaps_node = mat_node->first_node("TextureMaps");
        if(texmaps_node)
        {
            // For each texturemap
            for(xml_node<>* texmap_node=texmaps_node->first_node("TextureMap");
                texmap_node;
                texmap_node=texmap_node->next_sibling("TextureMap"))
            {
                std::string texmap_name, texmap_path;
                xml::parse_attribute(texmap_node, "name", texmap_name);
                xml::parse_attribute(texmap_node, "path", texmap_path);
                int index = texmap_names_to_index[H_(texmap_name.c_str())];
                entry.has_map[index] = true;
                entry.paths[index] = QString::fromStdString(texmap_path);
            }
        }
        xml_node<>* unis_node = mat_node->first_node("Uniforms");
        if(unis_node)
        {
            // For each uniform
            for(xml_node<>* uni_node=unis_node->first_node("Uniform");
                uni_node;
                uni_node=uni_node->next_sibling("Uniform"))
            {
                // TODO
                std::string uni_name, uni_val;
                xml::parse_attribute(uni_node, "name", uni_name);
                xml::parse_attribute(uni_node, "value", uni_val);
                //std::cout << uni_name << ": " << uni_val << std::endl;
            }
        }

        // Insert descriptor
        texture_descriptors_.insert(std::pair(hentryname, entry));
        // Populate texture list
        texlist_model_->append(entry.name);
    }
    texlist_sort_proxy_model_->sort(0);

    project_save_requested(false);
}

static void node_add_attribute(xml_document<>& doc, xml_node<>* node, const char* attr_name, const char* attr_val)
{
    char* al_attr_name = doc.allocate_string(attr_name);
    char* al_attr_val = doc.allocate_string(attr_val);
    xml_attribute<>* attr = doc.allocate_attribute(al_attr_name, al_attr_val);
    node->append_attribute(attr);
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
        {
            // Material node with name attribute
            xml_node<>* mat_node = doc.allocate_node(node_element, "Material");
            node_add_attribute(doc, mat_node, "name", entry.name.toUtf8().constData());
            node_add_attribute(doc, mat_node, "width", std::to_string(entry.width).c_str());
            node_add_attribute(doc, mat_node, "height", std::to_string(entry.height).c_str());

            // TextureMaps node to hold texture maps paths
            xml_node<>* texmap_node = doc.allocate_node(node_element, "TextureMaps");
            for(int ii=0; ii<NTEXMAPS; ++ii)
            {
                if(entry.has_map[ii])
                {
                    xml_node<>* tex_node = doc.allocate_node(node_element, "TextureMap");
                    node_add_attribute(doc, tex_node, "name", texmap_names[ii].c_str());
                    node_add_attribute(doc, tex_node, "path", entry.paths[ii].toUtf8().constData());

                    texmap_node->append_node(tex_node);
                }
            }
            // Uniforms node to hold uniform data
            xml_node<>* unis_node = doc.allocate_node(node_element, "Uniforms");
            if(!entry.has_map[0]) // No albedo map
            {
                xml_node<>* uni_node = doc.allocate_node(node_element, "Uniform");
                node_add_attribute(doc, uni_node, "name", texmap_names[0].c_str());
                node_add_attribute(doc, uni_node, "value", "(0,0,0)"); // TMP

                unis_node->append_node(uni_node);
            }
            if(!entry.has_map[1]) // No roughness map
            {
                xml_node<>* uni_node = doc.allocate_node(node_element, "Uniform");
                node_add_attribute(doc, uni_node, "name", texmap_names[1].c_str());
                node_add_attribute(doc, uni_node, "value", "0.2"); // TMP

                unis_node->append_node(uni_node);
            }
            if(!entry.has_map[2]) // No metallic map
            {
                xml_node<>* uni_node = doc.allocate_node(node_element, "Uniform");
                node_add_attribute(doc, uni_node, "name", texmap_names[2].c_str());
                node_add_attribute(doc, uni_node, "value", "0"); // TMP

                unis_node->append_node(uni_node);
            }
            if(!entry.has_map[3]) // No AO map
            {
                xml_node<>* uni_node = doc.allocate_node(node_element, "Uniform");
                node_add_attribute(doc, uni_node, "name", texmap_names[3].c_str());
                node_add_attribute(doc, uni_node, "value", "1"); // TMP

                unis_node->append_node(uni_node);
            }

            mat_node->append_node(texmap_node);
            mat_node->append_node(unis_node);

            mats_node->append_node(mat_node);
        }

        root->append_node(mats_node);

        std::ofstream outfile;
        outfile.open(filepath.toUtf8().constData());
        outfile << doc;

        //std::cout << doc << std::endl;

        current_project_ = project_name;
        project_save_requested(false);
    }
}

void EditorModel::close_project()
{
    save_project();
    DLOGN("Closing project: <n>" + current_project_.toStdString() + "</n>", "core", Severity::LOW);
    clear();
    project_save_requested(false);
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
