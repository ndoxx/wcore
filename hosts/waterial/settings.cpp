#include <QApplication>
#include "settings.h"

namespace waterial
{

WSettings::WSettings(const QDir& ini_dir):
QSettings(ini_dir.filePath("waterial.ini"), QSettings::IniFormat)
{

}

} // namespace waterial
