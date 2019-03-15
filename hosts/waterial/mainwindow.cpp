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
#include <QSpacerItem>
#include <QLineEdit>
#include <QKeyEvent>
#include <QMenu>
#include <QToolBar>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QFileDialog>
#include <QCheckBox>
#include <QProgressBar>
#include <QStatusBar>
#include <QImage>
#include <QFileInfo>
#include <QApplication>

#include "mainwindow.h"
#include "editor_model.h"
#include "texlist_delegate.h"
#include "texmap_controls.h"
#include "texmap_generator.h"
#include "new_project_dialog.h"
#include "droplabel.h"
#include "gl_widget.h"

// wcore
#include "config.h"
#include "logger.h"


using namespace wcore;

namespace medit
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
pjname_label_(new QLabel),
gl_widget_(new GLWidget),
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
    setWindowState(Qt::WindowMaximized);

    // Status bar
    create_status_bar();

    // Toolbars
    create_toolbars();

    QFrame* side_panel = new QFrame();
    side_panel->setObjectName("SidePanel");

    // Layouts
    QHBoxLayout* hlayout_main = new QHBoxLayout();
    QVBoxLayout* vlayout_side_panel = new QVBoxLayout(side_panel);
    QHBoxLayout* hlayout_sp_nt = new QHBoxLayout();
    QGridLayout* layout_main_panel = new QGridLayout();

    // * Setup side panel
    QPushButton* button_new_tex = new QPushButton(tr("New texture"));
    texname_edit_->clearFocus();

    hlayout_sp_nt->addWidget(texname_edit_);
    hlayout_sp_nt->addWidget(button_new_tex);

    vlayout_side_panel->addLayout(hlayout_sp_nt);
    vlayout_side_panel->addWidget(tex_list_);
    vlayout_side_panel->addWidget(dir_hierarchy_);

    side_panel->setMaximumWidth(300);

    // * Setup main panel
    // Texture maps
    texmap_controls_.push_back(new AlbedoControl());
    texmap_controls_.push_back(new RoughnessControl());
    texmap_controls_.push_back(new MetallicControl());
    texmap_controls_.push_back(new DepthControl());
    texmap_controls_.push_back(new AOControl());
    texmap_controls_.push_back(new NormalControl());

    for(int ii=0; ii<texmap_controls_.size(); ++ii)
    {
        int col = ii%3;
        int row = ii/3;
        layout_main_panel->addWidget(texmap_controls_[ii], row, col);
        layout_main_panel->setRowStretch(row, 1); // So that all texmap controls will stretch the same way
        layout_main_panel->setColumnStretch(col, 1);
    }

    // Preview
    QGroupBox* gb_preview_ctl = new QGroupBox(tr("Preview controls"));
    gb_preview_ctl->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    gl_widget_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    layout_main_panel->addWidget(gb_preview_ctl, 0, 3);
    layout_main_panel->addWidget(gl_widget_, 1, 3); // DNW black widget
    layout_main_panel->setColumnStretch(3, 3);

    // * Setup main layout
    hlayout_main->addWidget(side_panel);
    hlayout_main->addLayout(layout_main_panel);

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

    QObject::connect(tex_list_delegate_, SIGNAL(sig_data_changed()),
                     this,               SLOT(handle_project_needs_saving()));

    static_cast<NormalControl*>(texmap_controls_[NORMAL])->connect_controls(this);
    static_cast<AOControl*>(texmap_controls_[AO])->connect_controls(this);

    clear_view();
}

MainWindow::~MainWindow()
{
    delete dir_fs_model_;
    delete window_;
    delete editor_model_;
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

    toolbar_->addAction(QIcon(":/res/icons/new_project.png"), "New project",
                        this, SLOT(handle_new_project()));
    toolbar_->addAction(QIcon(":/res/icons/open_project.png"), "Open project",
                        this, SLOT(handle_open_project()));
    toolbar_->addAction(QIcon(":/res/icons/save.png"), "Save project",
                        this, SLOT(handle_serialize_project()));
    toolbar_->addAction(QIcon(":/res/icons/save_as.png"), "Save project as",
                        this, SLOT(handle_serialize_project_as()));
    toolbar_->addAction(QIcon(":/res/icons/close_project.png"), "Close project",
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
    // Retrieve texture map info from controls and write to texture entry
    for(int ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
        texmap_controls_[ii]->write_entry(entry);

    // TMP Set dimensions to first defined texture dimensions
    for(int ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
    {
        if(entry.texture_maps[ii]->has_image)
        {
            entry.width  = texmap_controls_[ii]->get_droplabel()->getPixmap().width();
            entry.height = texmap_controls_[ii]->get_droplabel()->getPixmap().height();
            break;
        }
    }
}

void MainWindow::update_texture_view()
{
    // * Update drop labels
    TextureEntry& entry = editor_model_->get_current_texture_entry();

    for(int ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
        texmap_controls_[ii]->read_entry(entry);
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
        // Indicate Qt that project needs to be saved
        setWindowModified(true);
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
        if(editor_model_->get_num_entries() == 0)
            clear_view();
        setWindowModified(true);
    }
}

void MainWindow::handle_clear_current_texture()
{
    DLOGW("NOT IMPLEMENTED YET", "core", Severity::WARN);
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
    context_menu.addAction(QIcon(":/res/icons/rename.png"), tr("&Rename"), this, SLOT(handle_rename_current_texture()));
    context_menu.addAction(QIcon(":/res/icons/delete.png"), tr("&Delete"), this, SLOT(handle_delete_current_texture()));

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
    // First, save texture to editor model
    handle_save_current_texture();

    DLOGN("Compiling <h>all</h> textures.", "core", Severity::LOW);
    editor_model_->traverse_entries([&](TextureEntry& entry)
    {
        const QString& texname = entry.name;
        if(!texname.isEmpty())
            editor_model_->compile(texname);
    });
}

void MainWindow::handle_serialize_project()
{
    handle_save_current_texture();

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

void MainWindow::handle_close_project()
{
    editor_model_->close_project();
    update_window_title("");
    clear_view();
}

void MainWindow::handle_quit()
{
    QCoreApplication::quit();
}

void MainWindow::handle_project_needs_saving()
{
    setWindowModified(true);
}

void MainWindow::handle_gen_normal_map()
{
    // Get current entry
    const QString& texname = editor_model_->get_current_texture_name();
    if(!texname.isEmpty())
    {
        handle_save_current_texture();
        TextureEntry& entry = editor_model_->get_current_texture_entry();
        // Get depth map if any
        if(entry.texture_maps[DEPTH]->has_image && entry.texture_maps[DEPTH]->use_image)
        {
            QApplication::setOverrideCursor(Qt::WaitCursor);

            QString depth_path(entry.texture_maps[DEPTH]->path);
            QImage depthmap(depth_path);
            QImage normalmap(entry.width, entry.height, QImage::Format_RGBA8888);

            // Generate normal map
            generator::NormalGenOptions options;
            NormalControl* normal_control = static_cast<NormalControl*>(texmap_controls_[NORMAL]);
            normal_control->get_options(options);
            generator::normal_from_depth(depthmap, normalmap, options);
            // Blur/Sharpen
            generator::blur_sharp(normalmap, options.sigma);

            // Get directory of depth map
            QDir dir(QFileInfo(depth_path).absoluteDir());

            // Generate filename and save normal map
            QString filename = texname + "_norm_gen.png";
            QString normal_path(dir.filePath(filename));

            DLOGN("Saving generated <h>normal</h> map:", "core", Severity::LOW);
            DLOGI("<p>" + normal_path.toStdString() + "</p>", "core", Severity::LOW);

            normalmap.save(normal_path);

            // Display newly generated normal map
            entry.texture_maps[NORMAL]->has_image = true;
            entry.texture_maps[NORMAL]->use_image = true;
            entry.texture_maps[NORMAL]->path = normal_path;
            update_texture_view();

            QApplication::restoreOverrideCursor();
        }
    }
}

void MainWindow::handle_gen_ao_map()
{
    // Get current entry
    const QString& texname = editor_model_->get_current_texture_name();
    if(!texname.isEmpty())
    {
        handle_save_current_texture();
        TextureEntry& entry = editor_model_->get_current_texture_entry();
        // Get depth map if any
        if(entry.texture_maps[DEPTH]->has_image && entry.texture_maps[DEPTH]->use_image)
        {
            QApplication::setOverrideCursor(Qt::WaitCursor);

            QString depth_path(entry.texture_maps[DEPTH]->path);
            QImage depthmap(depth_path);
            QImage aomap(entry.width, entry.height, QImage::Format_RGBA8888);

            // Generate normal map
            generator::AOGenOptions options;
            AOControl* ao_control = static_cast<AOControl*>(texmap_controls_[AO]);
            ao_control->get_options(options);
            generator::ao_from_depth(depthmap, aomap, options);
            // Blur/Sharpen
            generator::blur_sharp(aomap, options.sigma);

            // Get directory of depth map
            QDir dir(QFileInfo(depth_path).absoluteDir());

            // Generate filename and save normal map
            QString filename = texname + "_ao_gen.png";
            QString ao_path(dir.filePath(filename));

            DLOGN("Saving generated <h>AO</h> map:", "core", Severity::LOW);
            DLOGI("<p>" + ao_path.toStdString() + "</p>", "core", Severity::LOW);

            aomap.save(ao_path);

            // Display newly generated normal map
            entry.texture_maps[AO]->has_image = true;
            entry.texture_maps[AO]->use_image = true;
            entry.texture_maps[AO]->path = ao_path;
            update_texture_view();

            QApplication::restoreOverrideCursor();
        }
    }
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
    for(uint32_t ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
        texmap_controls_[ii]->clear();
}


} // namespace medit
