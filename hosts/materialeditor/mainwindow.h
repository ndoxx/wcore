#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFrame>

class QTreeView;
class QListView;
class QLineEdit;
class QToolBar;
class QFileSystemModel;
class QItemSelection;
class QFileDialog;
class QGroupBox;
class QVBoxLayout;
class QCheckBox;

namespace medit
{

// Defines a control surface below the texmaps droplabels in a TexMapControl
class TexmapControlWidget: public QFrame
{
    Q_OBJECT

public:
    explicit TexmapControlWidget(QWidget* parent = nullptr);
    virtual ~TexmapControlWidget() = default;

    virtual void clear() {}
};

class ColorPickerLabel;
// Specialized controls for albedo map
class AlbedoControls: public TexmapControlWidget
{
    Q_OBJECT

public:
    explicit AlbedoControls(QWidget* parent = nullptr);
    virtual ~AlbedoControls() = default;

    virtual void clear() override;

    ColorPickerLabel* color_picker_;
};

class DropLabel;
// Groups all the controls for a given texture map
class TexMapControl: public QObject
{
    Q_OBJECT

public:
    QGroupBox* groupbox    = nullptr;
    QVBoxLayout* layout    = nullptr;
    DropLabel* droplabel   = nullptr;
    QCheckBox* map_enabled = nullptr;

public slots:
    void handle_sig_texmap_changed(bool init_state);
};

class EditorModel;
class TexlistDelegate;
class NewProjectDialog;
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
    // Add a new control panel for a texmap
    void push_texmap_control(const QString& title, QLayout* parent, QWidget* additional_controls=nullptr);

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
    void handle_project_save_state_changed(bool state);
    void handle_quit();

protected:
    void create_toolbars();
    void update_window_title(const QString& project_name, bool needs_saving);
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

    std::vector<TexMapControl*> texmap_controls_;
    TexmapControlWidget* albedo_controls_;

    // Dialogs
    NewProjectDialog* new_project_dialog_;
    QFileDialog* file_dialog_;
};


} // namespace medit

#endif // MAINWINDOW_H
