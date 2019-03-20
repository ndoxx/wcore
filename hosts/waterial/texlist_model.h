#ifndef TEXLIST_MODEL_H
#define TEXLIST_MODEL_H

#include <QStandardItemModel>

namespace waterial
{

class TexListModel: public QStandardItemModel
{
    Q_OBJECT

public:
    // To append data to list
    QModelIndex append(const QString& string);
    TexListModel& operator<<(const QString& string);
};

} // namespace waterial

#endif // TEXLIST_MODEL_H
