#include <locale>
#include <QApplication>
#include <QSurfaceFormat>

#include "mainwindow.h"
#include "config.h"

int main(int argc, char *argv[])
{
    wcore::CONFIG.init();

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
