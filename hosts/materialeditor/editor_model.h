#ifndef EDITOR_MODEL_H
#define EDITOR_MODEL_H

#include <map>
#include <array>
#include <QString>
#include <QDir>
#include <QStringList>

#include "wtypes.h"
#include "math3d.h"

/*
    Defines the behavior of the material editor application
*/

class QStringListModel;
class QSortFilterProxyModel;
class QListView;
namespace medit
{

#define NTEXMAPS 6

struct TextureMap
{
    TextureMap();
    virtual ~TextureMap() = default;
    virtual void parse_uniform_value(const std::string& value_str) {}
    virtual std::string uniform_value_string() { return ""; }
#ifdef __DEBUG__
    virtual void debug_display();
#endif

    QString path;
    bool texture_enabled;
};

struct AlbedoMap: public TextureMap
{
    virtual ~AlbedoMap() = default;
    virtual void parse_uniform_value(const std::string& value_str) override;
    virtual std::string uniform_value_string() override;
#ifdef __DEBUG__
    virtual void debug_display() override;
#endif

    wcore::math::vec4 u_albedo;
};

struct RoughnessMap: public TextureMap
{
    virtual ~RoughnessMap() = default;
    virtual void parse_uniform_value(const std::string& value_str) override;
    virtual std::string uniform_value_string() override;
#ifdef __DEBUG__
    virtual void debug_display() override;
#endif

    float u_roughness;
};

struct MetallicMap: public TextureMap
{
    virtual ~MetallicMap() = default;
    virtual void parse_uniform_value(const std::string& value_str) override;
    virtual std::string uniform_value_string() override;
#ifdef __DEBUG__
    virtual void debug_display() override;
#endif

    float u_metallic;
};

struct AOMap: public TextureMap
{
    virtual ~AOMap() = default;
    virtual void parse_uniform_value(const std::string& value_str) override;
    virtual std::string uniform_value_string() override;
#ifdef __DEBUG__
    virtual void debug_display() override;
#endif

    float u_ao;
};

struct DepthMap: public TextureMap
{
    virtual ~DepthMap() = default;
    virtual void parse_uniform_value(const std::string& value_str) override;
    virtual std::string uniform_value_string() override;
#ifdef __DEBUG__
    virtual void debug_display() override;
#endif
};

struct NormalMap: public TextureMap
{
    virtual ~NormalMap() = default;
    virtual void parse_uniform_value(const std::string& value_str) override;
    virtual std::string uniform_value_string() override;
#ifdef __DEBUG__
    virtual void debug_display() override;
#endif
};

struct TextureEntry
{
    TextureEntry();
    TextureEntry(const TextureEntry&);
    ~TextureEntry();

    wcore::hash_t parse_node(rapidxml::xml_node<>* mat_node);
    void write_node(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* materials_node);

    std::array<TextureMap*, NTEXMAPS> texture_maps;
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

    inline bool project_needs_saving() const { return needs_saving_; }
    inline const QString& get_current_project() const { return current_project_; }

signals:
    // Called when save request state has changed (save needed or save performed)
    void sig_save_requested_state(bool state);

protected:
    void project_save_requested(bool state);
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
    bool needs_saving_;
};

} // namespace medit

#endif // EDITOR_MODEL_H
