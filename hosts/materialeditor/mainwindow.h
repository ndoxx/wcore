#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFrame>
#include <QGroupBox>

class QTreeView;
class QListView;
class QLineEdit;
class QToolBar;
class QFileSystemModel;
class QItemSelection;
class QFileDialog;
class QProgressBar;
class QStatusBar;
class QLabel;

namespace medit
{

class EditorModel;
class TexlistDelegate;
class NewProjectDialog;
class TexMapControl;
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

    virtual bool eventFilter(QObject* object, QEvent* event) override;
    virtual void keyPressEvent(QKeyEvent* event) override;

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

    // Status bar
    QLabel* status_label_;
    //QProgressBar* status_progress_;

    // Dialogs
    NewProjectDialog* new_project_dialog_;
    QFileDialog* file_dialog_;
};


} // namespace medit

#endif // MAINWINDOW_H
