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
#include <QFrame>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QFileDialog>

#include "mainwindow.h"
#include "droplabel.h"
#include "editor_model.h"
#include "texlist_delegate.h"
#include "new_project_dialog.h"

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

static std::vector<TexMapControl> texmap_controls;

static void push_texmap_control(const QString& title, QLayout* parent)
{
    TexMapControl ctl;
    ctl.groupbox = new QGroupBox(title);
    ctl.layout = new QVBoxLayout();
    ctl.droplabel = new DropLabel();

    ctl.droplabel->setAcceptDrops(true);
    ctl.droplabel->setMinimumSize(QSize(128,128));

    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    ctl.droplabel->setSizePolicy(policy);

    ctl.layout->addSpacerItem(new QSpacerItem(20, 10));
    ctl.layout->addWidget(ctl.droplabel);
    ctl.layout->setAlignment(ctl.droplabel, Qt::AlignTop);
    ctl.groupbox->setLayout(ctl.layout);

    parent->addWidget(ctl.groupbox);
    texmap_controls.push_back(ctl);
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

MainWindow::MainWindow(QWidget* parent):
QMainWindow(parent),
editor_model_(new EditorModel),
dir_fs_model_(new QFileSystemModel),
window_(new QWidget),
dir_hierarchy_(new QTreeView),
tex_list_(new QListView),
texname_edit_(new QLineEdit),
tex_list_delegate_(new TexlistDelegate),
new_project_dialog_(new NewProjectDialog(this)),
file_dialog_(new QFileDialog(this))
{
    // Load style
    QFile stylesheet_file(":/res/stylesheets/arduino_style.css");
    if(stylesheet_file.open(QFile::ReadOnly | QFile::Text))
    {
        QTextStream in(&stylesheet_file);
        QString stylesheet = in.readAll();
        setStyleSheet(stylesheet);
    }

    window_->setWindowFlags(Qt::Window);
    window_->setObjectName("Window");
    update_window_title("", false);

    // Toolbars
    create_toolbars();

    QFrame* side_panel = new QFrame();
    side_panel->setObjectName("SidePanel");

    // Layouts
    QHBoxLayout* hlayout_main = new QHBoxLayout();
    QVBoxLayout* vlayout_side_panel = new QVBoxLayout(side_panel);
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

    side_panel->setMaximumWidth(300);

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
    hlayout_main->addWidget(side_panel);
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
    editor_model_->set_project_folder(work_path_qstr);

    QStringList project_extension_filters; // Filter files by extension
    project_extension_filters << "*.wmp";
    file_dialog_->setDirectory(work_path_qstr);
    file_dialog_->setNameFilters(project_extension_filters);

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
    QStringList extension_filters; // Filter files by extension
    extension_filters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp" << "*.gif";
    dir_fs_model_->setRootPath(work_path_qstr);
    dir_fs_model_->setNameFilters(extension_filters);
    dir_fs_model_->setNameFilterDisables(false); // Filtered out files are hidden
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
    QObject::connect(editor_model_, SIGNAL(sig_save_requested_state(bool)),
                     this,          SLOT(handle_project_save_state_changed(bool)));
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
    //toolbar_->setFixedHeight(50);

    toolbar_->addAction(/*QIcon(":/res/icons/new_project.png"), */"New project",
                        this, SLOT(handle_new_project()));
    toolbar_->addAction(/*QIcon(":/res/icons/open.png"), */"Open project",
                        this, SLOT(handle_open_project()));
    toolbar_->addAction(QIcon(":/res/icons/save.png"), "Save project",
                        this, SLOT(handle_serialize_project()));
    toolbar_->addAction(/*QIcon(":/res/icons/save_as.png"), */"Save project as",
                        this, SLOT(handle_serialize_project_as()));
    toolbar_->addAction(/*QIcon(":/res/icons/close.png"), */"Close project",
                        this, SLOT(handle_close_project()));

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
    toolbar_->addAction(QIcon(":/res/icons/compile_all.png"), "Compile all",
                        this, SLOT(handle_compile_all()));
}

bool MainWindow::eventFilter(QObject* object, QEvent* event)
{
    if(event->type() == QEvent::KeyPress)
    {
        QKeyEvent* ke = static_cast<QKeyEvent*>(event);
        if(ke->key() == Qt::Key_Delete && object == tex_list_)
        {
            handle_delete_current_texture();
            return true;
        }
        return false;
    }
    else
        return false;
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if(event->type() == QEvent::KeyPress)
    {
        if(event->matches(QKeySequence::New))
            handle_new_project();
        if(event->matches(QKeySequence::Open))
            handle_open_project();
        if(event->matches(QKeySequence::Save))
            handle_serialize_project();
        if(event->matches(QKeySequence::SaveAs))
            handle_serialize_project_as();
        if(event->matches(QKeySequence::Close))
            handle_close_project();
        if(event->matches(QKeySequence::Quit))
            handle_quit();
    }
}

void MainWindow::update_entry(TextureEntry& entry)
{
    // Retrieve texture map info from controls
    for(int ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
    {
        entry.texture_maps[ii]->path = texmap_controls[ii].droplabel->get_path();
        entry.texture_maps[ii]->texture_enabled = !entry.texture_maps[ii]->path.isEmpty();
    }
    // Set dimensions to first defined texture dimensions
    for(int ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
    {
        if(entry.texture_maps[ii]->texture_enabled)
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
    TextureEntry& entry = editor_model_->get_current_texture_entry();

    for(int ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
    {
        texmap_controls[ii].droplabel->clear();
        if(entry.texture_maps[ii]->texture_enabled)
            texmap_controls[ii].droplabel->setPixmap(entry.texture_maps[ii]->path);
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
    {
        DLOGN("Saving texture <n>" + texname.toStdString() + "</n>", "core", Severity::LOW);

        TextureEntry& entry = editor_model_->get_current_texture_entry();
        entry.name = texname;
        update_entry(entry);
    }
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

void MainWindow::handle_serialize_project()
{
    handle_save_current_texture();

    // If project is the unnamed project, use save as instead
    if(!editor_model_->get_current_project().isEmpty())
        editor_model_->save_project();
    else
        handle_serialize_project_as();
}

void MainWindow::handle_serialize_project_as()
{
    // TODO: handle project overwrite
    auto ret = new_project_dialog_->exec();
    if(ret == QDialog::Accepted)
    {
        QString project_name = new_project_dialog_->get_project_name();
        if(editor_model_->validate_project_name(project_name))
        {
            editor_model_->save_project_as(project_name);
        }
    }
}

void MainWindow::handle_new_project()
{
    // TODO: handle project overwrite
    // if new project name is an existing project let user choose btw:
    //  -> Overwrite
    //  -> Open
    //  -> Cancel
    auto ret = new_project_dialog_->exec();
    if(ret == QDialog::Accepted)
    {
        QString project_name = new_project_dialog_->get_project_name();
        if(editor_model_->validate_project_name(project_name))
        {
            // Check if current project is the unnamed project
            bool is_unnamed = editor_model_->get_current_project().isEmpty();
            editor_model_->new_project(project_name);
            // Clear view only if an actual project was closed
            if(!is_unnamed)
                clear_view();
            // Serialize newly created project
            handle_serialize_project();
        }
    }
}

void MainWindow::handle_open_project()
{
    // Close current project if any and clear view
    handle_close_project();

    if(file_dialog_->exec())
    {
        QString filename = file_dialog_->selectedFiles().first();
        editor_model_->open_project(filename);
        // Select first texture by default (shows the user something happened)
        QModelIndex index = tex_list_->model()->index(0,0);
        if(index.isValid())
            tex_list_->selectionModel()->select(index, QItemSelectionModel::Select);
    }
}

void MainWindow::handle_close_project()
{
    editor_model_->close_project();
    clear_view();
}

void MainWindow::handle_project_save_state_changed(bool state)
{
    update_window_title(editor_model_->get_current_project(), state);
}

void MainWindow::handle_quit()
{
    QCoreApplication::quit();
}

void MainWindow::update_window_title(const QString& project_name, bool needs_saving)
{
    QString title = "WCore Material Editor - ";
    if(project_name.isEmpty())
        title += "[unnamed project]";
    else
        title += project_name;
    if(needs_saving)
        title += " *";

    setWindowTitle(title);
}

void MainWindow::clear_view()
{
    // * Clear drop labels
    for(uint32_t ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
    {
        texmap_controls[ii].droplabel->clear();
    }
}


} // namespace medit
