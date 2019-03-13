#include "texlist_model.h"

namespace medit
{

QModelIndex TexListModel::append(const QString& string)
{
    insertRows(rowCount(), 1);
    setData(index(rowCount()-1), string);
    return index(rowCount()-1);
}

TexListModel& TexListModel::operator<<(const QString& string)
{
    append(string);
    return *this;
}

} // namespace medit
