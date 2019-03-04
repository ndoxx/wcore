#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QTreeView;
class QListView;
class QLineEdit;
class QFileSystemModel;
class QItemSelection;

namespace medit
{

class EditorModel;
class TexlistDelegate;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void save_texture(const QString& texname);
    void update_texture_view();

    virtual bool eventFilter(QObject* object, QEvent* event) override;


public slots:
    void handle_new_texture();
    void handle_delete_current_texture();
    void handle_rename_current_texture();
    void handle_save_current_texture();
    void handle_texlist_selection_changed(const QItemSelection& selection);
    void handle_texlist_context_menu(const QPoint& pos);

private:
    EditorModel* editor_model_;
    QFileSystemModel* dir_fs_model_;
    QWidget* window_;
    QTreeView* dir_hierarchy_;
    QListView* tex_list_;
    QLineEdit* texname_edit_;
    TexlistDelegate* tex_list_delegate_;
};


} // namespace medit

#endif // MAINWINDOW_H