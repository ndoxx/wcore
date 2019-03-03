#include "texlist_model.h"

namespace medit
{

void TexListModel::append(const QString& string)
{
    insertRows(rowCount(), 1);
    setData(index(rowCount()-1), string);
}

TexListModel& TexListModel::operator<<(const QString& string)
{
    append(string);
    return *this;
}

} // namespace medit
