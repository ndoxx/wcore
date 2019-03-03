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
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void save_texture(const QString& texname);
    void update_texture_view();

public slots:
    void handle_new_texture();
    void handle_save_current_texture();
    void handle_texlist_selection_changed(const QItemSelection& selection);

private:
    EditorModel* editor_model_;
    QFileSystemModel* dir_fs_model_;
    QWidget* window_;
    QTreeView* dir_hierarchy_;
    QListView* tex_list_;
    QLineEdit* texname_edit_;
};


} // namespace medit

#endif // MAINWINDOW_H
