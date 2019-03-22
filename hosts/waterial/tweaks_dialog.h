#ifndef TWEAKS_DIALOG_H
#define TWEAKS_DIALOG_H

#include <QDialog>

#include "math3d.h"

namespace waterial
{

QT_FORWARD_DECLARE_CLASS(TweaksGLWidget)

class TweaksDialog: public QDialog
{
    Q_OBJECT

public:
    explicit TweaksDialog(QWidget* parent=nullptr);
    ~TweaksDialog();

    void set_clear_color(const wcore::math::vec3& value);
    void set_source_image(const QString& path);

private:
    TweaksGLWidget* preview_;
};

} // namespace waterial

#endif
