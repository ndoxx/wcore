#include <iostream>
#include <vector>
#include <cassert>
#include <QFileSystemModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QTreeView>
#include <QListView>
#include <QLineEdit>
#include <QKeyEvent>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QStatusBar>
#include <QApplication>
#include <QTabWidget>
#include <QProgressDialog>

#include "mainwindow.h"
#include "editor_model.h"
#include "texlist_delegate.h"
#include "texmap_controls.h"
#include "preview_controls.h"
#include "new_project_dialog.h"
#include "droplabel.h"
#include "double_slider.h"
#include "preview_gl_widget.h"
#include "settings.h"

// wcore
#include "config.h"
#include "logger.h"

using namespace wcore;

namespace waterial
{

static bool validate_texture_name(const QString& name)
{
    if(name.isEmpty()) return false;

    // Check that name is alphanumeric (underscores are allowed)
    QRegularExpression re("^[a-zA-Z0-9_]+$");
    QRegularExpressionMatch match = re.match(name);

    bool ret = match.hasMatch();

    if(!ret)
    {
        DLOGW("Invalid texture name.", "waterial");
        DLOGI("<h>Rules</h>: alphanumeric, no space, underscores allowed.", "waterial");
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
pjname_label_(new QLabel),
main_tab_widget_(new QTabWidget),
gl_widget_(new PreviewGLWidget),
preview_controls_(new PreviewControlWidget(gl_widget_)),
texmap_pane_(new TexmapControlPane(this, editor_model_)),
new_project_dialog_(new NewProjectDialog(this)),
file_dialog_(new QFileDialog(this))
{
    // Load style
    QFile stylesheet_file(":/res/stylesheets/default_style.css");
    if(stylesheet_file.open(QFile::ReadOnly | QFile::Text))
    {
        QTextStream in(&stylesheet_file);
        QString stylesheet = in.readAll();
        setStyleSheet(stylesheet);
    }

    window_->setWindowFlags(Qt::Window);
    window_->setObjectName("Window");
    pjname_label_->setObjectName("TBProjectLabel");
    update_window_title("");
    setWindowIcon(QIcon(":/res/icons/waterial.ico"));
    //setWindowState(Qt::WindowMaximized);


    // Get config folder
    fs::path conf_path = CONFIG.get_config_directory();
    if(fs::exists(conf_path))
        config_folder_ = QDir(QString::fromStdString(conf_path.string()));
    else
    {
        DLOGW("Unable to locate config directory.", "waterial");
        DLOGI("Using current directory to store configuration", "waterial");
        config_folder_ = QDir::currentPath();
    }

    read_settings();
    create_actions();
    update_recent_file_actions();
    create_menus();
    create_status_bar();
    create_toolbars();

    QFrame* side_panel = new QFrame();
    side_panel->setObjectName("SidePanel");

    // Layouts
    QHBoxLayout* hlayout_main = new QHBoxLayout();
    QVBoxLayout* vlayout_side_panel = new QVBoxLayout(side_panel);
    QHBoxLayout* hlayout_sp_nt = new QHBoxLayout();

    // * Setup side panel
    QPushButton* button_new_tex = new QPushButton(tr("New texture"));
    texname_edit_->clearFocus();

    hlayout_sp_nt->addWidget(texname_edit_);
    hlayout_sp_nt->addWidget(button_new_tex);

    tex_list_->setIconSize(QSize(50, 50));
    tex_list_->setSpacing(2);

    vlayout_side_panel->addLayout(hlayout_sp_nt);
    vlayout_side_panel->addWidget(tex_list_);
    vlayout_side_panel->addWidget(dir_hierarchy_);

    side_panel->setMaximumWidth(300);

    // * Setup main panel
    QGridLayout* layout_main_panel = new QGridLayout();

    // Main tab widget
    main_tab_widget_->addTab(texmap_pane_, "Texture maps");
    main_tab_widget_->addTab(new QWidget(), "General");

    layout_main_panel->addWidget(main_tab_widget_, 0, 0, 2, 1);
    layout_main_panel->setColumnStretch(0, 8);

    // Preview
    preview_controls_->setObjectName("PreviewControls");
    preview_controls_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    gl_widget_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    layout_main_panel->addWidget(preview_controls_, 0, 1);
    layout_main_panel->addWidget(gl_widget_, 1, 1);
    layout_main_panel->setColumnStretch(1, 7);

    // * Setup main layout
    hlayout_main->addWidget(side_panel);
    hlayout_main->addLayout(layout_main_panel);

    window_->setLayout(hlayout_main);

    setCentralWidget(window_);

    // * Setup directory hierarchy view to display texture work folder
    // Get texture working directory from config
    fs::path work_path;
    QString work_path_qstr;
    if(!CONFIG.get<fs::path>("root.folders.matswork"_h, work_path))
    {
        DLOGW("Unable to read root.folders.matswork node in config.", "waterial");
        DLOGI("Using current directory instead.", "waterial");
        work_path_qstr = QDir::currentPath();
    }
    else
    {
        DLOGN("Detected texture work directory:", "waterial");
        DLOGI("<p>" + work_path.string() + "</p>", "waterial");
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
        DLOGW("Unable to read root.folders.texture node in config.", "waterial");
        DLOGI("Using current directory instead.", "waterial");
        editor_model_->set_output_folder(QDir::currentPath());
    }
    else
    {
        DLOGN("Detected texture output directory:", "waterial");
        DLOGI("<p>" + tex_path.string() + "</p>", "waterial");
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
    connect(button_new_tex, SIGNAL(clicked()),
            this,           SLOT(handle_new_texture()));
    connect(texname_edit_,  SIGNAL(returnPressed()),
            this,           SLOT(handle_new_texture()));
    connect(tex_list_->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this,                        SLOT(handle_texlist_selection_changed(QItemSelection)));
    connect(tex_list_, SIGNAL(customContextMenuRequested(const QPoint&)),
            this,      SLOT(handle_texlist_context_menu(const QPoint&)));

    connect(tex_list_delegate_, SIGNAL(sig_data_changed()),
            this,               SLOT(handle_project_needs_saving()));

    clear_view();

    // Open last project if any
    if(recent_files_.count())
    {
        recent_file_action_[0]->trigger();
    }
}

MainWindow::~MainWindow()
{
    delete dir_fs_model_;
    delete window_;
    delete editor_model_;
}

void MainWindow::read_settings()
{
    WSettings settings(config_folder_);
    restoreGeometry(settings.value("window/geometry").toByteArray());
    restoreState(settings.value("window/windowState").toByteArray());

    recent_files_ = settings.value("file/recent").toStringList();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Close project cleanly
    handle_close_project();

    // Save settings before closing
    DLOGN("Saving settings.", "waterial");
    WSettings settings(config_folder_);
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/windowState", saveState());
    settings.setValue("file/recent", recent_files_);
    QMainWindow::closeEvent(event);
}

void MainWindow::create_actions()
{
    // * File actions
    new_project_action_ = new QAction(tr("New project"), this);
    new_project_action_->setIcon(QIcon(":/res/icons/new_project.png"));
    new_project_action_->setStatusTip(tr("Create a new empty project"));
    new_project_action_->setShortcut(QKeySequence::New);
    connect(new_project_action_, SIGNAL(triggered()), this, SLOT(handle_new_project()));

    open_project_action_ = new QAction(tr("Open"), this);
    open_project_action_->setIcon(QIcon(":/res/icons/open_project.png"));
    open_project_action_->setStatusTip(tr("Open a project file"));
    open_project_action_->setShortcut(QKeySequence::Open);
    connect(open_project_action_, SIGNAL(triggered()), this, SLOT(handle_open_project()));

    save_project_action_ = new QAction(tr("Save"), this);
    save_project_action_->setIcon(QIcon(":/res/icons/save.png"));
    save_project_action_->setStatusTip(tr("Save current project"));
    save_project_action_->setShortcut(QKeySequence::Save);
    connect(save_project_action_, SIGNAL(triggered()), this, SLOT(handle_serialize_project()));

    save_project_as_action_ = new QAction(tr("Save as"), this);
    save_project_as_action_->setIcon(QIcon(":/res/icons/save_as.png"));
    save_project_as_action_->setStatusTip(tr("Save current project under another name"));
    save_project_as_action_->setShortcut(QKeySequence::SaveAs);
    connect(save_project_as_action_, SIGNAL(triggered()), this, SLOT(handle_serialize_project_as()));

    close_project_action_ = new QAction(tr("Close"), this);
    close_project_action_->setIcon(QIcon(":/res/icons/close_project.png"));
    close_project_action_->setStatusTip(tr("Close current project"));
    close_project_action_->setShortcut(QKeySequence::Close);
    connect(close_project_action_, SIGNAL(triggered()), this, SLOT(handle_close_project()));

    close_app_action_ = new QAction(tr("Exit"), this);
    close_app_action_->setStatusTip(tr("Quit Waterial"));
    close_app_action_->setShortcut(QKeySequence::Quit);
    connect(close_app_action_, SIGNAL(triggered()), this, SLOT(handle_quit()));

    for(int ii=0; ii<MAX_RECENT_FILES; ++ii)
    {
        recent_file_action_.push_back(new QAction("", this));
        connect(recent_file_action_[ii], SIGNAL(triggered()), this, SLOT(handle_open_recent_project()));
    }

    // * Current texture actions
    rename_tex_action_ = new QAction(tr("Rename"), this);
    rename_tex_action_->setIcon(QIcon(":/res/icons/rename.png"));
    rename_tex_action_->setStatusTip(tr("Rename current texture"));
    connect(rename_tex_action_, SIGNAL(triggered()), this, SLOT(handle_rename_current_texture()));

    clear_tex_action_ = new QAction(tr("Clear"), this);
    clear_tex_action_->setIcon(QIcon(":/res/icons/clear.png"));
    clear_tex_action_->setStatusTip(tr("Clear current texture"));
    connect(clear_tex_action_, SIGNAL(triggered()), this, SLOT(handle_clear_current_texture()));

    delete_tex_action_ = new QAction(tr("Delete"), this);
    delete_tex_action_->setIcon(QIcon(":/res/icons/delete.png"));
    delete_tex_action_->setStatusTip(tr("Delete current texture"));
    connect(delete_tex_action_, SIGNAL(triggered()), this, SLOT(handle_delete_current_texture()));
/*
    compile_tex_action_ = new QAction(tr("Compile"), this);
    compile_tex_action_->setIcon(QIcon(":/res/icons/compile.png"));
    compile_tex_action_->setStatusTip(tr("Compile current texture"));
    connect(compile_tex_action_, SIGNAL(triggered()), this, SLOT(handle_compile_current()));

    compile_all_tex_action_ = new QAction(tr("Compile All"), this);
    compile_all_tex_action_->setIcon(QIcon(":/res/icons/compile_all.png"));
    compile_all_tex_action_->setStatusTip(tr("Compile all textures in current project"));
    connect(compile_all_tex_action_, SIGNAL(triggered()), this, SLOT(handle_compile_all()));
*/
    compile_wat_tex_action_ = new QAction(tr("Compile Wat"), this);
    compile_wat_tex_action_->setIcon(QIcon(":/res/icons/compile.png"));
    compile_wat_tex_action_->setStatusTip(tr("Compile current texture to wat format"));
    connect(compile_wat_tex_action_, SIGNAL(triggered()), this, SLOT(handle_compile_wat_current()));

    compile_wat_all_tex_action_ = new QAction(tr("Compile Wat All"), this);
    compile_wat_all_tex_action_->setIcon(QIcon(":/res/icons/compile_all.png"));
    compile_wat_all_tex_action_->setStatusTip(tr("Compile all textures in current project to wat format"));
    connect(compile_wat_all_tex_action_, SIGNAL(triggered()), this, SLOT(handle_compile_wat_all()));
}

void MainWindow::update_recent_file_actions()
{
    QMutableStringListIterator it(recent_files_);
    // Remove all non existing files from list
    while(it.hasNext())
    {
        if(!QFile::exists(it.next()))
            it.remove();
    }

    // Add an action for each recent file discovered
    for(int ii=0; ii<MAX_RECENT_FILES; ++ii)
    {
        if(ii<recent_files_.count())
        {
            QString text = tr("&%1 %2").arg(ii+1).arg(recent_files_[ii]);
            recent_file_action_[ii]->setText(text);
            recent_file_action_[ii]->setData(recent_files_[ii]);
            recent_file_action_[ii]->setVisible(true);
        }
        else
            recent_file_action_[ii]->setVisible(false);
    }
}

void MainWindow::create_menus()
{
    // * File menu
    QMenu* file_menu = menuBar()->addMenu(tr("&File"));
    file_menu->addAction(new_project_action_);
    file_menu->addAction(open_project_action_);
    file_menu->addAction(save_project_action_);
    file_menu->addAction(save_project_as_action_);
    file_menu->addAction(close_project_action_);

    file_menu->addSeparator();
    // Recent file actions
    for(int ii=0; ii<MAX_RECENT_FILES; ++ii)
    {
        file_menu->addAction(recent_file_action_[ii]);
    }

    file_menu->addSeparator();

    file_menu->addAction(close_app_action_);

    // * Texture menu
    QMenu* tex_menu = menuBar()->addMenu(tr("&Texture"));
    tex_menu->addAction(rename_tex_action_);
    tex_menu->addAction(clear_tex_action_);
    tex_menu->addAction(delete_tex_action_);

    tex_menu->addSeparator();

    //tex_menu->addAction(compile_tex_action_);
    //tex_menu->addAction(compile_all_tex_action_);
    tex_menu->addAction(compile_wat_tex_action_);
    tex_menu->addAction(compile_wat_all_tex_action_);
}

void MainWindow::create_status_bar()
{
    status_label_ = new QLabel("plop");
    //status_progress_ = new QProgressBar;

    //status_progress_->setTextVisible(false);
    //status_progress_->setMaximumWidth(100);
    status_label_->setObjectName("StatusLabel");
    statusBar()->addWidget(status_label_);
    //status_bar_->addPermanentWidget(status_progress_);
}

void MainWindow::create_toolbars()
{
    toolbar_ = addToolBar("Texture");
    toolbar_->setObjectName("Toolbar");

    toolbar_->addAction(new_project_action_);
    toolbar_->addAction(open_project_action_);
    toolbar_->addAction(save_project_action_);
    toolbar_->addAction(save_project_as_action_);
    toolbar_->addAction(close_project_action_);

    toolbar_->addSeparator();

    toolbar_->addAction(rename_tex_action_);
    toolbar_->addAction(clear_tex_action_);
    toolbar_->addAction(delete_tex_action_);

    toolbar_->addSeparator();

    //toolbar_->addAction(compile_tex_action_);
    //toolbar_->addAction(compile_all_tex_action_);
    toolbar_->addAction(compile_wat_tex_action_);
    toolbar_->addAction(compile_wat_all_tex_action_);

    toolbar_->addSeparator();

    QFont font = pjname_label_->font();
    font.setPointSize(14);
    font.setBold(true);
    pjname_label_->setFont(font);
    pjname_label_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
    pjname_label_->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    toolbar_->addWidget(pjname_label_);
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
    /*if(event->type() == QEvent::KeyPress)
    {

    }*/
}

void MainWindow::handle_new_texture()
{
    QString newtex_name = texname_edit_->text();
    texname_edit_->clear();

    // First, check that no texture of the same name exists already
    if(editor_model_->has_entry(H_(newtex_name.toUtf8().constData())))
    {
        DLOGW("Texture <n>" + newtex_name.toStdString() + "</n> already exists.", "waterial");
        return;
    }

    // If name is valid, add texture to editor model
    if(validate_texture_name(newtex_name))
    {
        DLOGN("New texture: <n>" + newtex_name.toStdString() + "</n>", "waterial");
        QModelIndex index = editor_model_->add_texture(newtex_name);
        // Select newly created item in list view
        if(index.isValid())
        {
            tex_list_->selectionModel()->clearSelection();
            tex_list_->selectionModel()->select(index, QItemSelectionModel::Select);
        }
        // Indicate Qt that project needs to be saved
        setWindowModified(true);
    }
}

void MainWindow::handle_delete_current_texture()
{
    const QString& texname = editor_model_->get_current_texture_name();
    if(!texname.isEmpty())
    {
        DLOGN("Deleting texture <n>" + texname.toStdString() + "</n>", "waterial");
        // Remove from editor model
        editor_model_->delete_current_texture(tex_list_);
        if(editor_model_->get_num_entries() == 0)
            clear_view();
        setWindowModified(true);
    }
}

void MainWindow::handle_clear_current_texture()
{
    texmap_pane_->clear_view();
    setWindowModified(true);
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
        texmap_pane_->handle_save_current_texture();
        // Get newly selected item data as QString
        if(selection.indexes().first().isValid())
        {
            QString current_tex = selection.indexes().first().data(Qt::DisplayRole).toString();
            // Set editor model to work with this texture as current texture
            editor_model_->set_current_texture_name(current_tex);
            // Retrieve and display saved texture information
            texmap_pane_->update_texture_view();
            // Swap texture in viewer if possible
            handle_material_swap();
        }
    }
}

void MainWindow::handle_texlist_context_menu(const QPoint& pos)
{
    // Map widget coords to global coords
    // QListView uses QAbstractScrollArea, so we need to get its viewport coords
    QPoint globalPos = tex_list_->viewport()->mapToGlobal(pos);

    QMenu context_menu;
    context_menu.addAction(QIcon(":/res/icons/rename.png"), tr("&Rename"), this, SLOT(handle_rename_current_texture()));
    context_menu.addAction(QIcon(":/res/icons/delete.png"), tr("&Delete"), this, SLOT(handle_delete_current_texture()));

    context_menu.exec(globalPos);
}

void MainWindow::handle_compile_current()
{
    // First, save texture to editor model
    texmap_pane_->handle_save_current_texture();

    const QString& texname = editor_model_->get_current_texture_name();
    if(!texname.isEmpty())
    {
        editor_model_->compile(texname);
        // Swap material in preview
        handle_material_swap();
        // Update thumbnail in list
        const TextureEntry& entry = editor_model_->get_current_texture_entry();
        if(entry.texture_maps[ALBEDO]->has_image)
        {
            QModelIndex index = tex_list_->currentIndex();
            editor_model_->update_thumbnail_proxy(index, entry.texture_maps[ALBEDO]->source_path);
        }
    }
}

void MainWindow::handle_compile_all()
{
    // First, save texture to editor model
    texmap_pane_->handle_save_current_texture();

    DLOGN("Compiling <h>all</h> textures.", "waterial");
    QProgressDialog progress(this);
    progress.setRange(0, editor_model_->get_num_entries());
    progress.setModal(true);
    progress.setValue(0);
    progress.show();
    int count = 0;
    editor_model_->traverse_entries([&](TextureEntry& entry)
    {
        const QString& texname = entry.name;
        if(!texname.isEmpty())
        {
            qApp->processEvents();
            if(progress.wasCanceled())
                return false;
            progress.setLabelText(tr("Compiling %1").arg(texname));
            editor_model_->compile(texname);
            progress.setValue(++count);
        }
        return true;
    });
}

void MainWindow::handle_compile_wat_current()
{
    // First, save texture to editor model
    texmap_pane_->handle_save_current_texture();

    const QString& texname = editor_model_->get_current_texture_name();
    if(!texname.isEmpty())
    {
        editor_model_->compile(texname, true);
        // Swap material in preview
        handle_material_swap();
        // Update thumbnail in list
        const TextureEntry& entry = editor_model_->get_current_texture_entry();
        if(entry.texture_maps[ALBEDO]->has_image)
        {
            QModelIndex index = tex_list_->currentIndex();
            editor_model_->update_thumbnail_proxy(index, entry.texture_maps[ALBEDO]->source_path);
        }
    }
}

void MainWindow::handle_compile_wat_all()
{
    // First, save texture to editor model
    texmap_pane_->handle_save_current_texture();

    DLOGN("Compiling <h>all</h> textures.", "waterial");
    QProgressDialog progress(this);
    progress.setRange(0, editor_model_->get_num_entries());
    progress.setModal(true);
    progress.setValue(0);
    progress.show();
    int count = 0;
    editor_model_->traverse_entries([&](TextureEntry& entry)
    {
        const QString& texname = entry.name;
        if(!texname.isEmpty())
        {
            qApp->processEvents();
            if(progress.wasCanceled())
                return false;
            progress.setLabelText(tr("Compiling %1").arg(texname));
            editor_model_->compile(texname, true);
            progress.setValue(++count);
        }
        return true;
    });
}

void MainWindow::handle_serialize_project()
{
    texmap_pane_->handle_save_current_texture();

    // If project is the unnamed project, use save as instead
    if(!editor_model_->get_current_project().isEmpty())
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        editor_model_->save_project();
        QApplication::restoreOverrideCursor();
        setWindowModified(false);
    }
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
            QApplication::setOverrideCursor(Qt::WaitCursor);
            editor_model_->save_project_as(project_name);
            QApplication::restoreOverrideCursor();
            update_window_title(project_name);
            setWindowModified(false);
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
            update_window_title(project_name);

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
    if(file_dialog_->exec())
    {
        // Close current project if any and clear view
        handle_close_project();

        QString filename = file_dialog_->selectedFiles().first();
        QApplication::setOverrideCursor(Qt::WaitCursor);
        editor_model_->open_project(filename);
        QApplication::restoreOverrideCursor();
        update_window_title(editor_model_->get_current_project());

        // Select first texture by default (shows the user something happened)
        QModelIndex index = tex_list_->model()->index(0,0);
        if(index.isValid())
            tex_list_->selectionModel()->select(index, QItemSelectionModel::Select);
    }
}

void MainWindow::handle_open_recent_project()
{
    // Close current project if any and clear view
    handle_close_project();

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QAction* action = qobject_cast<QAction*>(sender());
    if(action)
    {
        editor_model_->open_project(action->data().toString());
        update_window_title(editor_model_->get_current_project());

        // Select first texture by default (shows the user something happened)
        QModelIndex index = tex_list_->model()->index(0,0);
        if(index.isValid())
            tex_list_->selectionModel()->select(index, QItemSelectionModel::Select);
    }
    QApplication::restoreOverrideCursor();
}

void MainWindow::handle_close_project()
{
    // Add recent file
    recent_files_.prepend(editor_model_->get_current_project_path());
    update_recent_file_actions();

    editor_model_->close_project();
    update_window_title("");
    clear_view();
}

void MainWindow::handle_quit()
{
    close();
}

void MainWindow::handle_project_needs_saving()
{
    setWindowModified(true);
}

void MainWindow::handle_material_swap()
{
    const QString& texname = editor_model_->get_current_texture_name();
    if(!texname.isEmpty())
        gl_widget_->handle_material_swap(editor_model_);
}

void MainWindow::update_window_title(const QString& project_name)
{
    setWindowTitle(tr("%1 - %2 [*]").arg(tr("Waterial"))
                                    .arg(project_name.isEmpty() ? tr("[unnamed project]") : project_name));
    pjname_label_->setText(project_name);
}

void MainWindow::clear_view()
{
    // Clear all texmap controls
    texmap_pane_->clear_view();
}


} // namespace waterial
