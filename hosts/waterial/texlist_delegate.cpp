#include <iostream>
#include <QListView>
#include <QLineEdit>

#include "texlist_delegate.h"
#include "editor_model.h"
#include "logger.h"

using namespace wcore;

namespace waterial
{

TexlistDelegate::TexlistDelegate(QObject* parent):
QItemDelegate(parent),
editor_model_(nullptr)
{

}

void TexlistDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QLineEdit* line_editor = qobject_cast<QLineEdit*>(editor);
    QString old_name = index.model()->data(index, Qt::EditRole).toString();
    QString new_name = line_editor->text();

    // Check that the name has indeed changed
    if(!old_name.compare(new_name)) return;

    // Validate name and commit
    if(item_name_validator_(new_name))
    {
        hash_t new_hname = H_(new_name.toUtf8().constData());
        if(!editor_model_->has_entry(new_hname))
        {
            DLOGN("Renaming texture <n>" + old_name.toStdString() + "</n> to <v>" + new_name.toStdString() + "</v>", "waterial");

            editor_model_->rename_texture(old_name, new_name);
            model->setData(index, QVariant::fromValue(new_name));
            emit sig_data_changed();
        }
        else
        {
            DLOGW("Texture <n>" + new_name.toStdString() + "</n> already exists.", "waterial");
        }
    }
}


} // namespace waterial
