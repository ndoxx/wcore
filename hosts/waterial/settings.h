#ifndef SETTINGS_H
#define SETTINGS_H
#include <QSettings>
#include <QDir>

namespace waterial
{

class WSettings: public QSettings
{
    Q_OBJECT

public:
    WSettings(const QDir& ini_dir);

};

} // namespace waterial

#endif
