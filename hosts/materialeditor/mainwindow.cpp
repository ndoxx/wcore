#include <iostream>
#include <vector>
#include <cassert>
#include <QFileSystemModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QTreeView>
#include <QListView>
#include <QSpacerItem>
#include <QLineEdit>
#include <QKeyEvent>
#include <QMenu>
#include <QToolBar>

#include "mainwindow.h"
#include "droplabel.h"
#include "editor_model.h"
#include "texlist_delegate.h"

// wcore
#include "config.h"
#include "logger.h"


using namespace wcore;

namespace medit
{

enum TexMapControlIndex: uint32_t
{
    ALBEDO,
    ROUGHNESS,
    METALLIC,
    AO,
    DEPTH,
    NORMAL,
    N_CONTROLS
};

struct TexMapControl
{
    QGroupBox* groupbox  = nullptr;
    QVBoxLayout* layout  = nullptr;
    DropLabel* droplabel = nullptr;
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

static std::vector<TexMapControl> texmap_controls;

static void push_texmap_control(const QString& title, QLayout* parent)
{
    TexMapControl ctl;
    ctl.groupbox = new QGroupBox(title);
    ctl.layout = new QVBoxLayout();
    ctl.droplabel = new DropLabel();

    ctl.droplabel->setAcceptDrops(true);
    ctl.droplabel->setMinimumSize(QSize(128,128));

    ctl.layout->addSpacerItem(new QSpacerItem(20, 10));
    ctl.layout->addWidget(ctl.droplabel);
    ctl.layout->setAlignment(ctl.droplabel, Qt::AlignTop);
    ctl.groupbox->setLayout(ctl.layout);
    ctl.groupbox->setStyleSheet(gbStyleSheet);

    parent->addWidget(ctl.groupbox);
    texmap_controls.push_back(ctl);
}

static void retrieve_texmap_path(TexMapControlIndex index, QString& dest_path, bool& has_map)
{
    assert(index<TexMapControlIndex::N_CONTROLS && "TexMapControlIndex out of bounds.");
    dest_path = texmap_controls[index].droplabel->get_path();
    has_map = !dest_path.isEmpty();
}

static bool validate_texture_name(const QString& name)
{
    if(name.isEmpty()) return false;

    // Check that name is alphanumeric (underscores are allowed)
    QRegularExpression re("^[a-zA-Z0-9_]+$");
    QRegularExpressionMatch match = re.match(name);

    bool ret = match.hasMatch();

    if(!ret)
    {
        DLOGW("Invalid texture name.", "core", Severity::WARN);
        DLOGI("<h>Rules</h>: alphanumeric, no space, underscores are allowed.", "core", Severity::WARN);
    }

    return ret;
}

MainWindow::MainWindow(QWidget *parent) :
QMainWindow(parent),
editor_model_(new EditorModel),
dir_fs_model_(new QFileSystemModel),
window_(new QWidget),
dir_hierarchy_(new QTreeView),
tex_list_(new QListView),
texname_edit_(new QLineEdit),
tex_list_delegate_(new TexlistDelegate)
{
    window_->setWindowFlags(Qt::Window);
    setWindowTitle("WCore Material Editor");

    // Toolbars
    create_toolbars();

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
    // Side panel list view will use model and data from editor model
    editor_model_->setup_list_model(tex_list_);
    tex_list_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tex_list_->installEventFilter(this);
    tex_list_->setContextMenuPolicy(Qt::CustomContextMenu);
    tex_list_delegate_->set_editor_model(editor_model_);
    tex_list_delegate_->set_item_name_validator(&validate_texture_name);
    tex_list_->setItemDelegate(tex_list_delegate_); // For editing purposes

    // * Configure signals and slots
    QObject::connect(button_new_tex, SIGNAL(clicked()),
                     this,           SLOT(handle_new_texture()));
    QObject::connect(texname_edit_,  SIGNAL(returnPressed()),
                     this,           SLOT(handle_new_texture()));
    QObject::connect(tex_list_->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                     this,                        SLOT(handle_texlist_selection_changed(QItemSelection)));
    QObject::connect(tex_list_, SIGNAL(customContextMenuRequested(const QPoint&)),
                     this,      SLOT(handle_texlist_context_menu(const QPoint&)));
}

MainWindow::~MainWindow()
{
    delete dir_fs_model_;
    delete window_;
    delete editor_model_;
}

void MainWindow::create_toolbars()
{
    toolbar_ = addToolBar("Texture");
    toolbar_->addAction(QIcon(":/res/icons/save.png"), "Save",
                        this, SLOT(handle_serialize()));
    toolbar_->addAction(QIcon(":/res/icons/save_all.png"), "Save All",
                        this, SLOT(handle_serialize_all()));

    toolbar_->addSeparator();

    toolbar_->addAction(QIcon(":/res/icons/rename.png"), "Rename",
                        this, SLOT(handle_rename_current_texture()));
    toolbar_->addAction(QIcon(":/res/icons/clear.png"), "Clear",
                        this, SLOT(handle_clear_current_texture()));
    toolbar_->addAction(QIcon(":/res/icons/delete.png"), "Delete",
                        this, SLOT(handle_delete_current_texture()));

    toolbar_->addSeparator();

    toolbar_->addAction(QIcon(":/res/icons/compile.png"), "Compile",
                        this, SLOT(handle_compile_current()));
    toolbar_->addAction(QIcon(":/res/icons/compile_all.png"), "Compile All",
                        this, SLOT(handle_compile_all()));
}

bool MainWindow::eventFilter(QObject* object, QEvent* event)
{
    if (object == tex_list_ && event->type() == QEvent::KeyPress)
    {
        QKeyEvent* ke = static_cast<QKeyEvent*>(event);
        if(ke->key() == Qt::Key_Delete)
        {
            handle_delete_current_texture();
            return true;
        }
        return false;
    }
    else
        return false;
}

void MainWindow::save_texture(const QString& texname)
{
    // * Retrieve texture map paths from drop labels
    if(texname.isEmpty()) return;

    DLOGN("Saving texture <n>" + texname.toStdString() + "</n>", "core", Severity::LOW);

    hash_t hname = H_(texname.toUtf8().constData());
    TextureEntry& entry = editor_model_->get_texture_entry(hname);

    entry.name = texname;

    for(uint32_t ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
        retrieve_texmap_path(TexMapControlIndex(ii), entry.paths[ii], entry.has_map[ii]);

    // Set dimensions to first defined texture dimensions
    for(uint32_t ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
    {
        if(entry.has_map[ii])
        {
            entry.width  = texmap_controls[ii].droplabel->getPixmap().width();
            entry.height = texmap_controls[ii].droplabel->getPixmap().height();
            break;
        }
    }
}

void MainWindow::update_texture_view()
{
    // * Update drop labels
    const QString& current = editor_model_->get_current_texture_name();
    hash_t hname = H_(current.toUtf8().constData());
    TextureEntry& entry = editor_model_->get_texture_entry(hname);

    for(uint32_t ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
    {
        texmap_controls[ii].droplabel->clear();
        if(entry.has_map[ii])
            texmap_controls[ii].droplabel->setPixmap(entry.paths[ii]);
    }
}

void MainWindow::handle_new_texture()
{
    QString newtex_name = texname_edit_->text();
    texname_edit_->clear();

    // First, check that no texture of the same name exists already
    if(editor_model_->has_entry(H_(newtex_name.toUtf8().constData())))
    {
        DLOGW("Texture <n>" + newtex_name.toStdString() + "</n> already exists.", "core", Severity::WARN);
        return;
    }

    // If name is valid, add texture to editor model
    if(validate_texture_name(newtex_name))
    {
        DLOGN("New texture: <n>" + newtex_name.toStdString() + "</n>", "core", Severity::LOW);
        QModelIndex index = editor_model_->add_texture(newtex_name);
        // Select newly created item in list view
        if(index.isValid())
        {
            tex_list_->selectionModel()->clearSelection();
            tex_list_->selectionModel()->select(index, QItemSelectionModel::Select);
        }
    }
}

void MainWindow::handle_save_current_texture()
{
    const QString& texname = editor_model_->get_current_texture_name();
    if(!texname.isEmpty())
        save_texture(texname);
}

void MainWindow::handle_save_all_textures()
{
    DLOGW("NOT IMPLEMENTED YET", "core", Severity::WARN);
}

void MainWindow::handle_delete_current_texture()
{
    const QString& texname = editor_model_->get_current_texture_name();
    if(!texname.isEmpty())
    {
        DLOGN("Deleting texture <n>" + texname.toStdString() + "</n>", "core", Severity::LOW);
        // Remove from editor model
        editor_model_->delete_current_texture(tex_list_);
    }
}

void MainWindow::handle_clear_current_texture()
{
    DLOGW("NOT IMPLEMENTED YET", "core", Severity::WARN);
}

void MainWindow::handle_rename_current_texture()
{
    const QString& texname = editor_model_->get_current_texture_name();
    if(!texname.isEmpty())
    {
        // Edit texture item at current index
        QModelIndex index = tex_list_->currentIndex();
        if(index.isValid())
            tex_list_->edit(index);
    }
}

void MainWindow::handle_texlist_selection_changed(const QItemSelection& selection)
{
    if(!selection.indexes().isEmpty())
    {
        // First, save previously selected texture if any
        handle_save_current_texture();
        // Get newly selected item data as QString
        QString current_tex = selection.indexes().first().data(Qt::DisplayRole).toString();
        // Set editor model to work with this texture as current texture
        editor_model_->set_current_texture_name(current_tex);
        // Retrieve and display saved texture information
        update_texture_view();
    }
}

void MainWindow::handle_texlist_context_menu(const QPoint& pos)
{
    // Map widget coords to global coords
    // QListView uses QAbstractScrollArea, so we need to get its viewport coords
    QPoint globalPos = tex_list_->viewport()->mapToGlobal(pos);

    QMenu context_menu;
    context_menu.addAction("Rename", this, SLOT(handle_rename_current_texture()));
    context_menu.addAction("Delete", this, SLOT(handle_delete_current_texture()));

    context_menu.exec(globalPos);
}

void MainWindow::handle_compile_current()
{
    // First, save texture to editor model
    handle_save_current_texture();

    const QString& texname = editor_model_->get_current_texture_name();
    if(!texname.isEmpty())
        editor_model_->compile(texname);
}

void MainWindow::handle_compile_all()
{
    DLOGN("Compiling <h>all</h> textures.", "core", Severity::LOW);
    DLOGW("NOT IMPLEMENTED YET", "core", Severity::WARN);
}

void MainWindow::handle_serialize()
{
    DLOGW("NOT IMPLEMENTED YET", "core", Severity::WARN);
}

void MainWindow::handle_serialize_all()
{
    DLOGW("NOT IMPLEMENTED YET", "core", Severity::WARN);
}


} // namespace medit
