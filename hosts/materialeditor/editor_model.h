#ifndef EDITOR_MODEL_H
#define EDITOR_MODEL_H

#include <map>
#include <array>
#include <QString>
#include <QStringList>
#include <filesystem>

#include "wtypes.h"

/*
    Defines the behavior of the material editor application
*/

namespace fs = std::filesystem;

class QStringListModel;
class QSortFilterProxyModel;
class QListView;
namespace medit
{

struct TextureEntry
{
    QString name;

    std::array<QString, 6> paths;
    std::array<bool, 6> has_map;

    void debug_display();
};

class TexListModel;
class EditorModel
{
public:
    EditorModel();
    ~EditorModel();

    // current texture name
    void set_current_texture_name(const QString& name);
    inline const QString& get_current_texture_name() const { return current_texname_; }

    // composite textures output folder
    void set_output_folder(const QString& path);
    inline const fs::path& get_output_folder() const { return output_folder_; }

    // texture list access
    void setup_list_model(QListView* listview);
    QModelIndex add_texture(const QString& name);

    inline TextureEntry& get_texture_entry(wcore::hash_t name) { return texture_descriptors_.at(name); }
    bool has_entry(wcore::hash_t name);
    void delete_current_texture(QListView* tex_list);
    void rename_texture(const QString& old_name, const QString& new_name);

private:
    QString current_texname_;
    QStringList texlist_;
    fs::path output_folder_;
    TexListModel* texlist_model_;
    QSortFilterProxyModel* texlist_sort_proxy_model_;
    std::map<wcore::hash_t, TextureEntry> texture_descriptors_;
};

} // namespace medit

#endif // EDITOR_MODEL_H
