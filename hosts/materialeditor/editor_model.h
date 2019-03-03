#ifndef EDITOR_MODEL_H
#define EDITOR_MODEL_H

#include <QString>
#include <QStringList>
#include <filesystem>

/*
    Defines the behavior of the material editor application
*/

namespace fs = std::filesystem;

class QStringListModel;
class QListView;
namespace medit
{

class TexListModel;
class EditorModel
{
public:
    EditorModel();
    ~EditorModel();

    // current texture name
    inline void set_current_texture_name(const QString& name) { current_texname_ = name; }
    inline const QString& get_current_texture_name() const    { return current_texname_; }

    // composite textures output folder
    void set_output_folder(const QString& path);
    inline const fs::path& get_output_folder() const { return output_folder_; }

    // texture list access
    void setup_list_model(QListView* listview);
    void add_texture(const QString& name);

private:
    QString current_texname_;
    QStringList texlist_;
    fs::path output_folder_;
    TexListModel* texlist_model_;
};

} // namespace medit

#endif // EDITOR_MODEL_H
