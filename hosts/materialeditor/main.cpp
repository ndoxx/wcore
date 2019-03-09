#include <locale>
#include <QApplication>

#include "mainwindow.h"
#include "config.h"

int main(int argc, char *argv[])
{
    wcore::CONFIG.init();

    QApplication a(argc, argv);
    // Qt screws with locale settings on unices to "sniff out" the charset.
    // Reset locale to default (classic) to avoid parsing errors.
    std::locale::global(std::locale("C"));
    medit::MainWindow w;
    w.show();

    return a.exec();
}
