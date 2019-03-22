#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <vector>

#include <QMainWindow>
#include <QFrame>
#include <QGroupBox>
#include <QDir>
#include <QAction>

QT_FORWARD_DECLARE_CLASS(QTreeView)
QT_FORWARD_DECLARE_CLASS(QListView)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QToolBar)
QT_FORWARD_DECLARE_CLASS(QFileSystemModel)
QT_FORWARD_DECLARE_CLASS(QItemSelection)
QT_FORWARD_DECLARE_CLASS(QFileDialog)
QT_FORWARD_DECLARE_CLASS(QProgressBar)
QT_FORWARD_DECLARE_CLASS(QStatusBar)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QTabWidget)

namespace waterial
{

QT_FORWARD_DECLARE_CLASS(EditorModel)
QT_FORWARD_DECLARE_CLASS(TexlistDelegate)
QT_FORWARD_DECLARE_CLASS(NewProjectDialog)
QT_FORWARD_DECLARE_CLASS(TexmapControlPane)
QT_FORWARD_DECLARE_CLASS(PreviewGLWidget)
QT_FORWARD_DECLARE_CLASS(PreviewControlWidget)

struct TextureEntry;
class MainWindow: public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    virtual ~MainWindow();

    virtual bool eventFilter(QObject* object, QEvent* event) Q_DECL_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;

public slots:
    void handle_new_texture();
    void handle_delete_current_texture();
    void handle_rename_current_texture();
    void handle_clear_current_texture();
    void handle_texlist_selection_changed(const QItemSelection& selection);
    void handle_texlist_context_menu(const QPoint& pos);
    void handle_compile_current();
    void handle_compile_all();
    void handle_serialize_project();
    void handle_serialize_project_as();
    void handle_new_project();
    void handle_open_project();
    void handle_open_recent_project();
    void handle_close_project();
    void handle_quit();
    void handle_project_needs_saving();
    void handle_material_swap();

protected slots:
    virtual void closeEvent(QCloseEvent* event) Q_DECL_OVERRIDE;

protected:
    void read_settings();
    void update_recent_file_actions();
    void create_actions();
    void create_menus();
    void create_status_bar();
    void create_toolbars();
    void update_window_title(const QString& project_name);
    void clear_view();

private:
    EditorModel* editor_model_;
    QFileSystemModel* dir_fs_model_;
    QWidget* window_;
    QTreeView* dir_hierarchy_;
    QListView* tex_list_;
    QLineEdit* texname_edit_;
    QToolBar* toolbar_;
    TexlistDelegate* tex_list_delegate_;

    QLabel* pjname_label_;

    QTabWidget* main_tab_widget_;

    // Preview
    PreviewGLWidget* gl_widget_;
    PreviewControlWidget* preview_controls_;

    TexmapControlPane* texmap_pane_;

    // Status bar
    QLabel* status_label_;

    // Dialogs
    NewProjectDialog* new_project_dialog_;
    QFileDialog* file_dialog_;

    QStringList recent_files_;

    // Actions
    enum { MAX_RECENT_FILES = 5 };
    std::vector<QAction*> recent_file_action_;

    QAction* new_project_action_;
    QAction* open_project_action_;
    QAction* save_project_action_;
    QAction* save_project_as_action_;
    QAction* close_project_action_;
    QAction* close_app_action_;

    QAction* rename_tex_action_;
    QAction* clear_tex_action_;
    QAction* delete_tex_action_;
    QAction* compile_tex_action_;
    QAction* compile_all_tex_action_;

    QDir config_folder_;
};


} // namespace waterial

#endif // MAINWINDOW_H
