#include <iostream>
#include <vector>
#include <QFileSystemModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QTreeView>
#include <QListView>
#include <QSpacerItem>
#include <QLineEdit>

#include "mainwindow.h"
#include "droplabel.h"
#include "editor_model.h"

// wcore
#include "config.h"
#include "logger.h"


using namespace wcore;

namespace medit
{

struct TexMapControl
{
    QGroupBox* groupbox = nullptr;
    QVBoxLayout* layout = nullptr;
    QLabel* droplabel   = nullptr;
};

static QString gbStyleSheet =
"QGroupBox\
{\
    border: 2px solid gray;\
    background: rgb(200,200,200);\
}\
QGroupBox::title\
{\
    background-color: transparent;\
    subcontrol-position: top center;\
    padding:0px 10px;\
}";

static QString dlStyleSheet = "border-radius: 5px; background: white;";

static std::vector<TexMapControl> texmap_controls;

static void push_texmap_control(const QString& title, QLayout* parent)
{
    TexMapControl ctl;
    ctl.groupbox = new QGroupBox(title);
    ctl.layout = new QVBoxLayout();
    ctl.droplabel = new DropLabel();

    ctl.droplabel->setAcceptDrops(true);
    ctl.droplabel->setMinimumSize(QSize(128,128));
    ctl.droplabel->setStyleSheet(dlStyleSheet);

    ctl.layout->addSpacerItem(new QSpacerItem(20, 10));
    ctl.layout->addWidget(ctl.droplabel);
    ctl.layout->setAlignment(ctl.droplabel, Qt::AlignTop);
    ctl.groupbox->setLayout(ctl.layout);
    ctl.groupbox->setStyleSheet(gbStyleSheet);

    parent->addWidget(ctl.groupbox);
    texmap_controls.push_back(ctl);
}

MainWindow::MainWindow(QWidget *parent) :
QMainWindow(parent),
editor_model_(new EditorModel),
dir_fs_model_(new QFileSystemModel),
window_(new QWidget),
dir_hierarchy_(new QTreeView),
tex_list_(new QListView),
texname_edit_(new QLineEdit)
{
    window_->setWindowFlags(Qt::Window);
    setWindowTitle("WCore Material Editor");

    // Layouts
    QHBoxLayout* hlayout_main = new QHBoxLayout();
    QVBoxLayout* vlayout_side_panel = new QVBoxLayout();
    QHBoxLayout* hlayout_sp_nt = new QHBoxLayout();
    QVBoxLayout* vlayout_main_panel = new QVBoxLayout();
    QHBoxLayout* hlayout_tex_maps = new QHBoxLayout();
    QHBoxLayout* hlayout_preview = new QHBoxLayout();

    // * Setup side panel
    QPushButton* button_new_tex = new QPushButton("New texture");
    texname_edit_->clearFocus();

    hlayout_sp_nt->addWidget(texname_edit_);
    hlayout_sp_nt->addWidget(button_new_tex);

    vlayout_side_panel->addLayout(hlayout_sp_nt);
    vlayout_side_panel->addWidget(tex_list_);
    vlayout_side_panel->addWidget(dir_hierarchy_);

    // * Setup main panel
    // Texture maps
    push_texmap_control("Albedo",    hlayout_tex_maps);
    push_texmap_control("Roughness", hlayout_tex_maps);
    push_texmap_control("Metallic",  hlayout_tex_maps);
    push_texmap_control("AO",        hlayout_tex_maps);
    push_texmap_control("Depth",     hlayout_tex_maps);
    push_texmap_control("Normal",    hlayout_tex_maps);

    // Preview
    QGroupBox* gb_preview_ctl = new QGroupBox("Preview controls");
    QGroupBox* gb_preview     = new QGroupBox("Preview");
    hlayout_preview->addWidget(gb_preview_ctl);
    hlayout_preview->addWidget(gb_preview);

    // Setup layouts
    vlayout_main_panel->addLayout(hlayout_tex_maps);
    vlayout_main_panel->addLayout(hlayout_preview);

    // * Setup main layout
    hlayout_main->addLayout(vlayout_side_panel);
    hlayout_main->addLayout(vlayout_main_panel);
    hlayout_main->setSizeConstraint(QLayout::SetMinimumSize);

    window_->setLayout(hlayout_main);

    setCentralWidget(window_);


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
    // Get output texture directory
    fs::path tex_path;
    if(!CONFIG.get<fs::path>("root.folders.texture"_h, tex_path))
    {
        DLOGW("Unable to read root.folders.texture node in config.", "core", Severity::WARN);
        DLOGI("Using current directory instead.", "core", Severity::WARN);
        editor_model_->set_output_folder(QDir::currentPath());
    }
    else
    {
        DLOGN("Detected texture output directory:", "core", Severity::LOW);
        DLOGI("<p>" + tex_path.string() + "</p>", "core", Severity::LOW);
        editor_model_->set_output_folder(QString::fromStdString(tex_path.string()));
    }

    // * Setup directory view
    // Set treeView to work with QFileSystemModel and change current directory
    dir_fs_model_->setRootPath(work_path_qstr);
    dir_hierarchy_->setModel(dir_fs_model_);
    dir_hierarchy_->setRootIndex(dir_fs_model_->index(work_path_qstr));

    // Hide all columns (Size, Date modified...) except name
    for(int ii=1; ii<dir_fs_model_->columnCount(); ++ii)
        dir_hierarchy_->hideColumn(ii);

    // Configure drag behavior for dir hierarchy treeView
    dir_hierarchy_->setSelectionMode(QAbstractItemView::SingleSelection);
    dir_hierarchy_->setDragEnabled(true);
    dir_hierarchy_->viewport()->setAcceptDrops(false);
    dir_hierarchy_->setDropIndicatorShown(true);

    // * Setup texture list
    editor_model_->setup_list_model(tex_list_);

    // * Configure signals and slots
    QObject::connect(button_new_tex, SIGNAL(clicked()),
                     this,           SLOT(new_texture()));
    QObject::connect(texname_edit_,  SIGNAL(returnPressed()),
                     this,           SLOT(new_texture()));
}

MainWindow::~MainWindow()
{
    delete dir_fs_model_;
    delete window_;
    delete editor_model_;
}

static bool validate_texture_name(const QString& name)
{
    if(name.isEmpty()) return false;

    // Check that name is alphanumeric (underscores are allowed)
    QRegularExpression re("^[a-zA-Z0-9_]+$");
    QRegularExpressionMatch match = re.match(name);

    return match.hasMatch();
}

void MainWindow::new_texture()
{
    DLOGN("New texture.", "core", Severity::LOW);

    QString newtex_name = texname_edit_->text();
    texname_edit_->clear();

    if(validate_texture_name(newtex_name))
    {
        DLOGI(newtex_name.toUtf8().constData(), "core", Severity::LOW);
        editor_model_->add_texture(newtex_name);
    }
    else
    {
        DLOGW("Invalid texture name.", "core", Severity::WARN);
        DLOGI("<h>Rules</h>: alphanumeric, no space, underscores are allowed.", "core", Severity::WARN);
    }
}

} // namespace medit
