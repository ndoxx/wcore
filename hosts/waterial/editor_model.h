#ifndef EDITOR_MODEL_H
#define EDITOR_MODEL_H

#include <map>
#include <array>
#include <functional>
#include <QString>
#include <QDir>
#include <QStringList>

#include "vendor/rapidxml/rapidxml.hpp"
#include "wtypes.h"
#include "math3d.h"
#include "material_common.h"

/*
    Defines the behavior of the material editor application
*/

QT_FORWARD_DECLARE_CLASS(QStringListModel)
QT_FORWARD_DECLARE_CLASS(QSortFilterProxyModel)
QT_FORWARD_DECLARE_CLASS(QListView)

namespace medit
{

enum TexMapControlIndex: uint32_t
{
    ALBEDO,
    ROUGHNESS,
    METALLIC,
    DEPTH,
    AO,
    NORMAL,
    N_CONTROLS
};

struct TextureMap
{
    TextureMap();
    virtual ~TextureMap() = default;
    virtual void parse(rapidxml::xml_node<>* node) {}
    virtual void write(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* node) {}
#ifdef __DEBUG__
    virtual void debug_display();
#endif

    QString path;
    bool has_image;
    bool use_image;
};

struct AlbedoMap: public TextureMap
{
    virtual ~AlbedoMap() = default;
    virtual void parse(rapidxml::xml_node<>* node) override;
    virtual void write(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* node) override;
#ifdef __DEBUG__
    virtual void debug_display() override;
#endif

    wcore::math::i32vec4 u_albedo;
};

struct RoughnessMap: public TextureMap
{
    virtual ~RoughnessMap() = default;
    virtual void parse(rapidxml::xml_node<>* node) override;
    virtual void write(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* node) override;
#ifdef __DEBUG__
    virtual void debug_display() override;
#endif

    float u_roughness;
};

struct MetallicMap: public TextureMap
{
    virtual ~MetallicMap() = default;
    virtual void parse(rapidxml::xml_node<>* node) override;
    virtual void write(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* node) override;
#ifdef __DEBUG__
    virtual void debug_display() override;
#endif

    float u_metallic;
};

struct AOMap: public TextureMap
{
    virtual ~AOMap() = default;
    virtual void parse(rapidxml::xml_node<>* node) override;
    virtual void write(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* node) override;
#ifdef __DEBUG__
    virtual void debug_display() override;
#endif

    float u_ao;
    bool gen_invert;
    float gen_strength;
    float gen_mean;
    float gen_range;
    float gen_blursharp;
};

struct DepthMap: public TextureMap
{
    virtual ~DepthMap() = default;
    virtual void parse(rapidxml::xml_node<>* node) override;
    virtual void write(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* node) override;

#ifdef __DEBUG__
    virtual void debug_display() override;
#endif

    float u_parallax_scale;
};

struct NormalMap: public TextureMap
{
    virtual ~NormalMap() = default;
    virtual void parse(rapidxml::xml_node<>* node) override;
    virtual void write(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* node) override;

#ifdef __DEBUG__
    virtual void debug_display() override;
#endif

    int gen_filter;
    bool gen_invert_r;
    bool gen_invert_g;
    bool gen_invert_h;
    float gen_level;
    float gen_strength;
    float gen_blursharp;
};

struct TextureEntry
{
    TextureEntry();
    TextureEntry(const TextureEntry&);
    ~TextureEntry();

    wcore::hash_t parse_node(rapidxml::xml_node<>* mat_node);
    void write_node(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* materials_node);

    std::array<TextureMap*, TexMapControlIndex::N_CONTROLS> texture_maps;
    QString name;

    int width  = 0;
    int height = 0;

#ifdef __DEBUG__
    void debug_display();
#endif
};

class TexListModel;
class EditorModel: public QObject
{
    Q_OBJECT

public:
    EditorModel();
    ~EditorModel();

    // current texture name
    void set_current_texture_name(const QString& name);
    inline const QString& get_current_texture_name() const { return current_texname_; }
    inline wcore::hash_t get_current_texture_key() const   { return wcore::H_(current_texname_.toUtf8().constData()); }

    // composite textures output folder
    void set_output_folder(const QString& path);
    inline const QDir& get_output_folder() const { return output_folder_; }

    // texture list access
    void setup_list_model(QListView* listview);
    QModelIndex add_texture(const QString& name);

    inline TextureEntry& get_texture_entry(wcore::hash_t name) { return texture_descriptors_.at(name); }
    inline TextureEntry& get_current_texture_entry()           { return get_texture_entry(wcore::H_(current_texname_.toUtf8().constData())); }
    inline int get_num_entries() const { return texture_descriptors_.size(); }
    bool has_entry(wcore::hash_t name);
    void delete_current_texture(QListView* tex_list);
    void rename_texture(const QString& old_name, const QString& new_name);

    void compile(const QString& texname);

    // Project management
    void clear();
    void set_project_folder(const QString& path);
    void new_project(const QString& project_name);
    void open_project(const QString& infile);
    void close_project();
    bool save_project();
    void save_project_as(const QString& project_name);

    bool validate_project_name(const QString& name);

    inline const QString& get_current_project() const { return current_project_; }

    void traverse_entries(std::function<void(TextureEntry&)> func);

    // Get engine material descriptor for current material
    wcore::MaterialDescriptor get_current_material_descriptor();

protected:
    QString project_path_from_name(const QString& name);
    QString project_file_from_name(const QString& name);

private:
    QString current_texname_;
    QString current_project_;
    QStringList texlist_;
    QDir output_folder_;
    QDir project_folder_;
    TexListModel* texlist_model_;
    QSortFilterProxyModel* texlist_sort_proxy_model_;
    std::map<wcore::hash_t, TextureEntry> texture_descriptors_;
};

} // namespace medit

#endif // EDITOR_MODEL_H
