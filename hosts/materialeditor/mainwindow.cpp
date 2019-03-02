#include <iostream>

#include "mainwindow.h"
#include "ui_mainwindow.h"

// wcore
#include "config.h"
#include "logger.h"


using namespace wcore;

MainWindow::MainWindow(QWidget *parent) :
QMainWindow(parent),
ui(new Ui::MainWindow),
dir_fs_model_(new QFileSystemModel)
{
    ui->setupUi(this);

    // * Setup directory hierarchy view to display texture work folder
    // Get texture working directory from config
    fs::path work_path;
    QString work_path_qstr;
    if(!CONFIG.get<fs::path>("root.folders.texwork"_h, work_path))
    {
        DLOGW("Unable to read root.folders.texwork node in config.", "core", Severity::WARN);
        DLOGI("Using current directory instead.", "core", Severity::WARN);
        work_path_qstr = QDir::currentPath();
    }
    else
    {
        DLOGN("Detected texture work directory:", "core", Severity::LOW);
        DLOGI("<p>" + work_path.string() + "</p>", "core", Severity::LOW);
        work_path_qstr = QString::fromStdString(work_path.string());
    }

    // Set treeView to work with QFileSystemModel and change current directory
    dir_fs_model_->setRootPath(work_path_qstr);
    ui->dirHierarchy->setModel(dir_fs_model_);
    ui->dirHierarchy->setRootIndex(dir_fs_model_->index(work_path_qstr));

    // Hide all columns (Size, Date modified...) except name
    for (int ii=1; ii<dir_fs_model_->columnCount(); ++ii)
        ui->dirHierarchy->hideColumn(ii);

    // Configure drag behavior for dir hierarchy treeView
    ui->dirHierarchy->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->dirHierarchy->setDragEnabled(true);
    ui->dirHierarchy->viewport()->setAcceptDrops(false);
    ui->dirHierarchy->setDropIndicatorShown(true);

    // Configure drop behavior for image labels
    ui->albedoImage->setAcceptDrops(true);
    ui->metallicImage->setAcceptDrops(true);
    ui->roughnessImage->setAcceptDrops(true);
    ui->aoImage->setAcceptDrops(true);
    ui->depthImage->setAcceptDrops(true);
    ui->normalImage->setAcceptDrops(true);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete dir_fs_model_;
}
