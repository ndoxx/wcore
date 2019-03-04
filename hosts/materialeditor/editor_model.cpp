#include <QListView>
#include <QSortFilterProxyModel>

#include "editor_model.h"
#include "texlist_model.h"
#include "logger.h"

using namespace wcore;

namespace medit
{

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
    output_folder_ = fs::path(path.toUtf8().constData());
    if(!fs::exists(output_folder_))
    {
        DLOGE("Output texture folder does not exist:", "core", Severity::CRIT);
        DLOGI(output_folder_.string(), "core", Severity::CRIT);
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


} // namespace medit