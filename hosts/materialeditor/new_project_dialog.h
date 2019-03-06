#ifndef NEW_PROJECT_DIALOG_H
#define NEW_PROJECT_DIALOG_H

#include <QDialog>

class QLineEdit;
namespace medit
{

class NewProjectDialog: public QDialog
{
    Q_OBJECT

public:
    NewProjectDialog(QWidget* parent=0);

    QString get_project_name() const;

private:
    QLineEdit* name_input_;
};

} // namespace medit

#endif // NEW_PROJECT_DIALOG_H
