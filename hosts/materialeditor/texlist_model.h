#ifndef TEXLIST_MODEL_H
#define TEXLIST_MODEL_H

#include <QStringListModel>

namespace medit
{

class TexListModel: public QStringListModel
{
    Q_OBJECT

public:
    // To append data to list
    void append(const QString& string);
    TexListModel& operator<<(const QString& string);
};

} // namespace medit

#endif // TEXLIST_MODEL_H
