#include <vector>
#include <fstream>
#include <string>
#include <QListView>
#include <QImage>
#include <QSortFilterProxyModel>

#include "editor_model.h"
#include "texlist_model.h"
#include "logger.h"
#include "wat_loader.h"
#include "xml_parser.h"
#include "xml_utils.hpp"
#include "material_common.h"
#include "config.h"

#include "vendor/rapidxml/rapidxml_print.hpp"

using namespace wcore;
using namespace rapidxml;

namespace waterial
{

static std::map<int, std::string> texmap_names =
{
    {TexMapControlIndex::ALBEDO,    "Albedo"},
    {TexMapControlIndex::ROUGHNESS, "Roughness"},
    {TexMapControlIndex::METALLIC,  "Metallic"},
    {TexMapControlIndex::AO,        "AO"},
    {TexMapControlIndex::DEPTH,     "Depth"},
    {TexMapControlIndex::NORMAL,    "Normal"}
};

static std::map<hash_t, int> texmap_names_to_index =
{
    {"Albedo"_h,    TexMapControlIndex::ALBEDO},
    {"Roughness"_h, TexMapControlIndex::ROUGHNESS},
    {"Metallic"_h,  TexMapControlIndex::METALLIC},
    {"AO"_h,        TexMapControlIndex::AO},
    {"Depth"_h,     TexMapControlIndex::DEPTH},
    {"Normal"_h,    TexMapControlIndex::NORMAL}
};

static std::map<int, std::string> filter_names =
{
    {0, "sobel"},
    {1, "scharr"}
};

static std::map<hash_t, int> filter_names_to_index =
{
    {"sobel"_h, 0},
    {"scharr"_h, 1}
};

TextureMap::TextureMap():
has_image(false),
use_image(false),
has_tweak(false)
{

}

#ifdef __DEBUG__
void TextureMap::debug_display()
{

}
void AlbedoMap::debug_display()
{
    if(has_image) DLOGI("albedo: <p>"    + source_path.toStdString() + "</p>", "waterial");
}
void RoughnessMap::debug_display()
{
    if(has_image) DLOGI("roughness: <p>" + source_path.toStdString() + "</p>", "waterial");
}
void MetallicMap::debug_display()
{
    if(has_image) DLOGI("metallic: <p>"  + source_path.toStdString() + "</p>", "waterial");
}
void AOMap::debug_display()
{
    if(has_image) DLOGI("ao: <p>"        + source_path.toStdString() + "</p>", "waterial");
}
void DepthMap::debug_display()
{
    if(has_image) DLOGI("depth: <p>"     + source_path.toStdString() + "</p>", "waterial");
}
void NormalMap::debug_display()
{
    if(has_image) DLOGI("normal: <p>"    + source_path.toStdString() + "</p>", "waterial");
}
void TextureEntry::debug_display()
{
    DLOG("Texture <n>" + name.toStdString() + "</n>", "waterial", Severity::LOW);
    for(int ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
        texture_maps[ii]->debug_display();
}
#endif

TextureEntry::TextureEntry():
texture_maps({new AlbedoMap,
              new RoughnessMap,
              new MetallicMap,
              new DepthMap,
              new AOMap,
              new NormalMap})
{

}

TextureEntry::TextureEntry(const TextureEntry& other):
texture_maps({new AlbedoMap(*static_cast<AlbedoMap*>(other.texture_maps[ALBEDO])),
              new RoughnessMap(*static_cast<RoughnessMap*>(other.texture_maps[ROUGHNESS])),
              new MetallicMap(*static_cast<MetallicMap*>(other.texture_maps[METALLIC])),
              new DepthMap(*static_cast<DepthMap*>(other.texture_maps[DEPTH])),
              new AOMap(*static_cast<AOMap*>(other.texture_maps[AO])),
              new NormalMap(*static_cast<NormalMap*>(other.texture_maps[NORMAL]))}),
name(other.name),
width(other.width),
height(other.height)
{

}

TextureEntry::~TextureEntry()
{
    for(int ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
        delete texture_maps[ii];
}

hash_t TextureEntry::parse_node(xml_node<>* mat_node)
{
    // Get work directory from config so as to import absolute paths
    fs::path matswork;
    wcore::CONFIG.get("root.folders.matswork"_h, matswork);
    QDir workdir(QString::fromStdString(matswork.string()));

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
            std::string texmap_name, texmap_path, tweak_path;
            xml::parse_attribute(texmap_node, "name", texmap_name);
            int index = texmap_names_to_index[H_(texmap_name.c_str())];
            if(xml::parse_node(texmap_node, "Source", texmap_path))
            {
                texture_maps[index]->has_image = true;
                texture_maps[index]->source_path = workdir.absoluteFilePath(QString::fromStdString(texmap_path));
            }
            if(xml::parse_node(texmap_node, "Tweak", tweak_path))
            {
                texture_maps[index]->has_tweak = true;
                texture_maps[index]->tweak_path = workdir.absoluteFilePath(QString::fromStdString(tweak_path));
            }
            // Parse common per-map properties
            xml::parse_node(texmap_node, "TextureMapEnabled", texture_maps[index]->use_image);

            // Parse data specific to this map
            texture_maps[index]->parse(texmap_node);
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
    // Get work directory from config so as to export paths relative to it
    fs::path matswork;
    wcore::CONFIG.get("root.folders.matswork"_h, matswork);
    QDir workdir(QString::fromStdString(matswork.string()));

    // Material node with name attribute
    xml_node<>* mat_node = doc.allocate_node(node_element, "Material");
    node_add_attribute(doc, mat_node, "name", name.toUtf8().constData());
    node_add_attribute(doc, mat_node, "width", std::to_string(width).c_str());
    node_add_attribute(doc, mat_node, "height", std::to_string(height).c_str());

    // TextureMaps node to hold texture maps paths
    xml_node<>* texmap_node = doc.allocate_node(node_element, "TextureMaps");
    for(int ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
    {
        xml_node<>* tex_node = doc.allocate_node(node_element, "TextureMap");
        node_add_attribute(doc, tex_node, "name", texmap_names[ii].c_str());
        if(texture_maps[ii]->has_image)
        {
            // File path relative to work directory
            QString rel_source_path(workdir.relativeFilePath(texture_maps[ii]->source_path));

            xml_node<>* path_node = doc.allocate_node(node_element, "Source");
            node_set_value(doc, path_node, rel_source_path.toUtf8().constData());
            tex_node->append_node(path_node);

            if(texture_maps[ii]->has_tweak)
            {
                QString rel_tweak_path(workdir.relativeFilePath(texture_maps[ii]->tweak_path));

                xml_node<>* tweak_node = doc.allocate_node(node_element, "Tweak");
                node_set_value(doc, tweak_node, rel_tweak_path.toUtf8().constData());
                tex_node->append_node(tweak_node);
            }
        }

        // Save uniform value even if not used
        texture_maps[ii]->write(doc, tex_node);

        // Save per-map properties
        xml_node<>* useimg_node = doc.allocate_node(node_element, "TextureMapEnabled");
        node_set_value(doc, useimg_node, texture_maps[ii]->use_image ? "true" : "false");
        tex_node->append_node(useimg_node);

        texmap_node->append_node(tex_node);
    }

    mat_node->append_node(texmap_node);
    materials_node->append_node(mat_node);
}

void AlbedoMap::parse(rapidxml::xml_node<>* node)
{
    xml::parse_attribute(node, "value", u_albedo);
}

void RoughnessMap::parse(rapidxml::xml_node<>* node)
{
    xml::parse_attribute(node, "value", u_roughness);
}

void MetallicMap::parse(rapidxml::xml_node<>* node)
{
    xml::parse_attribute(node, "value", u_metallic);
}

void DepthMap::parse(rapidxml::xml_node<>* node)
{
    xml::parse_node(node, "ParallaxHeightScale", u_parallax_scale);
}

void AOMap::parse(rapidxml::xml_node<>* node)
{
    xml::parse_attribute(node, "value", u_ao);
}

void NormalMap::parse(rapidxml::xml_node<>* node)
{

}


void AlbedoMap::write(rapidxml::xml_document<>& doc, xml_node<>* node)
{
    // Save uniform value even if not used
    std::string value_str(wcore::to_string(u_albedo));
    if(value_str.size()!=0)
        node_add_attribute(doc, node, "value", value_str.c_str());
}

void RoughnessMap::write(rapidxml::xml_document<>& doc, xml_node<>* node)
{
    // Save uniform value even if not used
    std::string value_str(wcore::to_string(u_roughness));
    if(value_str.size()!=0)
        node_add_attribute(doc, node, "value", value_str.c_str());
}

void MetallicMap::write(rapidxml::xml_document<>& doc, xml_node<>* node)
{
    // Save uniform value even if not used
    std::string value_str(wcore::to_string(u_metallic));
    if(value_str.size()!=0)
        node_add_attribute(doc, node, "value", value_str.c_str());
}

void DepthMap::write(rapidxml::xml_document<>& doc, xml_node<>* node)
{
    xml_node<>* plxscale_node = doc.allocate_node(node_element, "ParallaxHeightScale");
    node_set_value(doc, plxscale_node, std::to_string(u_parallax_scale).c_str());
    node->append_node(plxscale_node);
}

void AOMap::write(rapidxml::xml_document<>& doc, xml_node<>* node)
{
    // Save uniform value even if not used
    std::string value_str(wcore::to_string(u_ao));
    if(value_str.size()!=0)
        node_add_attribute(doc, node, "value", value_str.c_str());
}

void NormalMap::write(rapidxml::xml_document<>& doc, xml_node<>* node)
{

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
        DLOGE("Output texture folder does not exist:", "waterial");
        DLOGI(path.toStdString(), "waterial");
    }
}

void EditorModel::set_current_texture_name(const QString& name)
{
    current_texname_ = name;
    DLOGN("Working on texture <n>" + current_texname_.toStdString() + "</n>", "waterial");
}

void EditorModel::setup_list_model(QListView* listview)
{
    // Proxy model for sorting the texture list view when a new item is added
    texlist_sort_proxy_model_->setDynamicSortFilter(true);
    texlist_sort_proxy_model_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    texlist_sort_proxy_model_->setFilterKeyColumn(0);
    texlist_sort_proxy_model_->setSourceModel(texlist_model_);
    listview->setModel(texlist_sort_proxy_model_);
}

void EditorModel::update_thumbnail(QModelIndex index, const QString& path)
{
    texlist_model_->setData(index, QIcon(path), Qt::DecorationRole);
}

void EditorModel::update_thumbnail_proxy(QModelIndex index, const QString& path)
{
    update_thumbnail(texlist_sort_proxy_model_->mapToSource(index), path);
}

QModelIndex EditorModel::add_texture(const QString& name, const TextureEntry& entry)
{
    // Append texture name to list view data and sort
    QModelIndex source_index = texlist_model_->append(name);

    // Update thumbnail
    if(entry.texture_maps[ALBEDO]->has_image)
        update_thumbnail(source_index, entry.texture_maps[ALBEDO]->source_path);
    else
        update_thumbnail(source_index, ":/res/icons/waterial.png");

    texlist_sort_proxy_model_->sort(0);

    // Add texture descriptor
    texture_descriptors_.insert(std::pair(H_(name.toUtf8().constData()), entry));

    // source index needs to be remapped to proxy sorted index
    return texlist_sort_proxy_model_->mapFromSource(source_index);
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
            QModelIndex index = tex_list->currentIndex();
            QModelIndex source_index = texlist_sort_proxy_model_->mapToSource(index);
            texlist_model_->removeRow(source_index.row());
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

void EditorModel::compile(const QString& texname, bool export_wat)
{
    DLOGN("Compiling texture <n>" + texname.toStdString() + "</n>", "waterial");

    hash_t hname = H_(texname.toUtf8().constData());
    TextureEntry& entry = get_texture_entry(hname);

    if(entry.width && entry.height)
    {
        DLOGI("Size: WxH= <v>" + std::to_string(entry.width) + "x" + std::to_string(entry.height) + "</v>", "waterial");

        QImage block0(entry.width, entry.height, QImage::Format_RGBA8888);
        QImage block1(entry.width, entry.height, QImage::Format_RGBA8888);
        QImage block2(entry.width, entry.height, QImage::Format_RGBA8888);

        QImage** texmaps = new QImage*[TexMapControlIndex::N_CONTROLS];
        for(int ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
        {
            if(entry.texture_maps[ii]->has_tweak)
                texmaps[ii] = new QImage(entry.texture_maps[ii]->tweak_path);
            else if(entry.texture_maps[ii]->has_image)
                texmaps[ii] = new QImage(entry.texture_maps[ii]->source_path);
            else
                texmaps[ii] = nullptr;
        }

        for(int xx=0; xx<entry.width; ++xx)
        {
            for(int yy=0; yy<entry.height; ++yy)
            {
                // Block 0
                QRgb albedo = texmaps[ALBEDO] ? texmaps[ALBEDO]->pixel(xx,yy) : qRgba(0,0,0,255);

                // Block 1
                QRgb normal = texmaps[NORMAL] ? texmaps[NORMAL]->pixel(xx,yy) : qRgb(0,0,0);
                int depth   = texmaps[DEPTH]  ? qRed(texmaps[DEPTH]->pixel(xx,yy)) : 0;

                // Block 2
                int metallic  = texmaps[METALLIC]  ? qRed(texmaps[METALLIC]->pixel(xx,yy)) : 0;
                int ao        = texmaps[AO]        ? qRed(texmaps[AO]->pixel(xx,yy)) : 0;
                int roughness = texmaps[ROUGHNESS] ? qRed(texmaps[ROUGHNESS]->pixel(xx,yy)) : 0;

                QRgb out_block1 = qRgba(qRed(normal), qGreen(normal), qBlue(normal), depth);
                QRgb out_block2 = qRgba(metallic, ao, roughness, 255);

                block0.setPixel(xx, yy, albedo);
                block1.setPixel(xx, yy, out_block1);
                block2.setPixel(xx, yy, out_block2);
            }
        }

        if(export_wat)
        {
            QString filename = texname + ".wat";

            MaterialDescriptor mat_info;
            AlbedoMap* albedo_map       = static_cast<AlbedoMap*>(entry.texture_maps[ALBEDO]);
            DepthMap* depth_map         = static_cast<DepthMap*>(entry.texture_maps[DEPTH]);
            MetallicMap* metallic_map   = static_cast<MetallicMap*>(entry.texture_maps[METALLIC]);
            RoughnessMap* roughness_map = static_cast<RoughnessMap*>(entry.texture_maps[ROUGHNESS]);

            if(entry.texture_maps[ALBEDO]->has_image)    mat_info.texture_descriptor.add_unit(TextureUnit::ALBEDO);
            if(entry.texture_maps[NORMAL]->has_image)    mat_info.texture_descriptor.add_unit(TextureUnit::NORMAL);
            if(entry.texture_maps[DEPTH]->has_image)     mat_info.texture_descriptor.add_unit(TextureUnit::DEPTH);
            if(entry.texture_maps[METALLIC]->has_image)  mat_info.texture_descriptor.add_unit(TextureUnit::METALLIC);
            if(entry.texture_maps[AO]->has_image)        mat_info.texture_descriptor.add_unit(TextureUnit::AO);
            if(entry.texture_maps[ROUGHNESS]->has_image) mat_info.texture_descriptor.add_unit(TextureUnit::ROUGHNESS);

            mat_info.texture_descriptor.block0_data = block0.bits();
            mat_info.texture_descriptor.block1_data = block1.bits();
            mat_info.texture_descriptor.block2_data = block2.bits();
            mat_info.texture_descriptor.resource_id = H_(filename.toStdString().c_str());
            mat_info.texture_descriptor.width       = entry.width;
            mat_info.texture_descriptor.height      = entry.height;

            wcore::math::vec3 u_albedo(albedo_map->u_albedo.x() / 255.f,
                                       albedo_map->u_albedo.y() / 255.f,
                                       albedo_map->u_albedo.z() / 255.f);
            mat_info.albedo                = u_albedo;
            mat_info.metallic              = metallic_map->u_metallic;
            mat_info.roughness             = roughness_map->u_roughness;
            mat_info.parallax_height_scale = depth_map->u_parallax_scale;
            mat_info.transparency          = 1; // TMP

            WatLoader watfile;
            std::ofstream stream(output_folder_.filePath(filename).toStdString(), std::ios::out | std::ios::binary);
            watfile.write(stream, mat_info);
            stream.close();
        }
        else
        {
            // Save blocks to png files
            QString block0_name = texname + "_block0.png";
            QString block1_name = texname + "_block1.png";
            QString block2_name = texname + "_block2.png";

            DLOGI("block0: <p>" + block0_name.toStdString() + "</p>", "waterial");
            DLOGI("block1: <p>" + block1_name.toStdString() + "</p>", "waterial");
            DLOGI("block2: <p>" + block2_name.toStdString() + "</p>", "waterial");

            block0.save(output_folder_.filePath(block0_name));
            block1.save(output_folder_.filePath(block1_name));
            block2.save(output_folder_.filePath(block2_name));
        }

        delete[] texmaps;
    }
}

void EditorModel::clear()
{
    current_texname_ = "";
    current_project_ = "";
    texture_descriptors_.clear();
    texlist_model_->clear();
}

void EditorModel::set_project_folder(const QString& path)
{
    project_folder_ = QDir(path);
    if(!project_folder_.exists())
    {
        DLOGE("Project folder does not exist:", "waterial");
        DLOGI(path.toStdString(), "waterial");
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
        DLOGN("New project: <n>" + project_name.toStdString() + "</n>", "waterial");
    }
}

void EditorModel::open_project(const QString& infile)
{
    DLOGN("Opening project:", "waterial");
    DLOGI("<p>" + infile.toStdString() + "</p>", "waterial");

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
        entry.parse_node(mat_node);
        // Insert descriptor
        add_texture(entry.name, entry);
    }
    texlist_sort_proxy_model_->sort(0);
}

void EditorModel::save_project_as(const QString& project_name)
{
    if(!project_name.isEmpty())
    {
        DLOGN("Saving project: <n>" + project_name.toStdString() + "</n>", "waterial");
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
    DLOGN("Closing project: <n>" + current_project_.toStdString() + "</n>", "waterial");
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
        DLOGW("Invalid project name.", "waterial");
        DLOGI("<h>Rules</h>: alphanumeric, no space, underscores allowed.", "waterial");
    }

    return ret;
}

QString EditorModel::project_path_from_name(const QString& name) const
{
    return project_folder_.filePath(project_file_from_name(name));
}

QString EditorModel::project_file_from_name(const QString& name) const
{
    return name + ".wmp";
}

void EditorModel::traverse_entries(std::function<bool(TextureEntry&)> func)
{
    for(auto&& [key, entry]: texture_descriptors_)
        if(!func(entry))
            break;
}

} // namespace waterial
