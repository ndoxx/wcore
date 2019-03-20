#include "texlist_model.h"

namespace waterial
{

QModelIndex TexListModel::append(const QString& string)
{
    insertRows(rowCount(), 1);
    //setData(index(rowCount()-1), string);
    appendRow(new QStandardItem(string));
    return index(rowCount()-1,0);
}

TexListModel& TexListModel::operator<<(const QString& string)
{
    append(string);
    return *this;
}

} // namespace waterial
