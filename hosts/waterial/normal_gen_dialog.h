#ifndef NORMAL_GEN_DIALOG_H
#define NORMAL_GEN_DIALOG_H

#include <QDialog>

namespace waterial
{

QT_FORWARD_DECLARE_CLASS(NormalGenGLWidget)

class NormalGenDialog: public QDialog
{
    Q_OBJECT

public:
    explicit NormalGenDialog(QWidget* parent=nullptr);
    ~NormalGenDialog();

public slots:
    void handle_accept();

private:
    NormalGenGLWidget* preview_;

private:

};

} // namespace waterial

#endif
