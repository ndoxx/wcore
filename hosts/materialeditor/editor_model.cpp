#include <QListView>

#include "editor_model.h"
#include "texlist_model.h"
#include "logger.h"

using namespace wcore;

namespace medit
{

EditorModel::EditorModel():
texlist_model_(new TexListModel)
{

}

EditorModel::~EditorModel()
{
    delete texlist_model_;
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

void EditorModel::setup_list_model(QListView* listview)
{
    texlist_model_->setStringList(texlist_);
    listview->setModel(texlist_model_);
}

void EditorModel::add_texture(const QString& name)
{
    texlist_model_->append(name);
    set_current_texture_name(name);
}


} // namespace medit
