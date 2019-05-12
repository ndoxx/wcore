#include <locale>
#include <QApplication>
#include <QSurfaceFormat>

#include "mainwindow.h"
#include "config.h"
#include "logger.h"

using namespace wcore;

int main(int argc, char *argv[])
{
    wcore::CONFIG.init();
    wcore::dbg::LOG.register_channel("waterial",  3);

    // Declare work folder for materials to config
    auto rootdir = wcore::CONFIG.get_root_directory();
    auto matswork = rootdir.parent_path() / "WCoreAssetsT/materials/";
    if(fs::exists(matswork))
        wcore::CONFIG.set("root.folders.matswork"_h, std::cref(matswork));
    else
        DLOGF("Materials work directory does not exist.", "waterial");

    QApplication a(argc, argv);
    // Qt screws with locale settings on unices to "sniff out" the charset.
    // Reset locale to default (classic) to avoid parsing errors.
    std::locale::global(std::locale("C"));

    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    fmt.setVersion(4, 0);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(fmt);

    waterial::MainWindow w;
    w.show();

    return a.exec();
}
