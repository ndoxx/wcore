#ifndef TWEAKS_DIALOG_H
#define TWEAKS_DIALOG_H

#include <QDialog>

namespace waterial
{

QT_FORWARD_DECLARE_CLASS(TweaksGLWidget)

class TweaksDialog: public QDialog
{
    Q_OBJECT

public:
    explicit TweaksDialog(QWidget* parent=nullptr);
    ~TweaksDialog();

private:
    TweaksGLWidget* preview_;
};

} // namespace waterial

#endif
