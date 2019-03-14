#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFrame>
#include <QGroupBox>

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

namespace medit
{

QT_FORWARD_DECLARE_CLASS(EditorModel)
QT_FORWARD_DECLARE_CLASS(TexlistDelegate)
QT_FORWARD_DECLARE_CLASS(NewProjectDialog)
QT_FORWARD_DECLARE_CLASS(TexMapControl)
QT_FORWARD_DECLARE_CLASS(GLWidget)

struct TextureEntry;
class MainWindow: public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    virtual ~MainWindow();

    // Retrieve data from current texture entry and update view
    void update_texture_view();
    // Retrieve data from controls and update a given entry with this information
    void update_entry(TextureEntry& entry);

    virtual bool eventFilter(QObject* object, QEvent* event) Q_DECL_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;

public slots:
    void handle_new_texture();
    void handle_delete_current_texture();
    void handle_rename_current_texture();
    void handle_clear_current_texture();
    void handle_save_current_texture();
    void handle_save_all_textures();
    void handle_texlist_selection_changed(const QItemSelection& selection);
    void handle_texlist_context_menu(const QPoint& pos);
    void handle_compile_current();
    void handle_compile_all();
    void handle_serialize_project();
    void handle_serialize_project_as();
    void handle_new_project();
    void handle_open_project();
    void handle_close_project();
    void handle_quit();
    void handle_project_needs_saving();
    void handle_gen_normal_map();
    void handle_gen_ao_map();

protected:
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

    std::vector<TexMapControl*> texmap_controls_;

    // Preview
    //GLWidget* gl_widget_;

    // Status bar
    QLabel* status_label_;

    // Dialogs
    NewProjectDialog* new_project_dialog_;
    QFileDialog* file_dialog_;
};


} // namespace medit

#endif // MAINWINDOW_H
