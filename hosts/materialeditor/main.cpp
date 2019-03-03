#include "mainwindow.h"
#include <QApplication>

#include "config.h"

int main(int argc, char *argv[])
{
    wcore::CONFIG.init();

    QApplication a(argc, argv);
    medit::MainWindow w;
    w.show();

    return a.exec();
}
