#ifndef NEW_PROJECT_DIALOG_H
#define NEW_PROJECT_DIALOG_H

#include <QDialog>

QT_FORWARD_DECLARE_CLASS(QLineEdit)

namespace waterial
{

class NewProjectDialog: public QDialog
{
    Q_OBJECT

public:
    NewProjectDialog(QWidget* parent=nullptr);

    QString get_project_name() const;

private:
    QLineEdit* name_input_;
};

} // namespace waterial

#endif // NEW_PROJECT_DIALOG_H
