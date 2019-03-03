#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QTreeView;
class QListView;
class QLineEdit;
class QFileSystemModel;

namespace medit
{

class EditorModel;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void new_texture();

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
