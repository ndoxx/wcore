#ifndef TWEAKS_DIALOG_H
#define TWEAKS_DIALOG_H

#include <QDialog>

#include "math3d.h"

namespace waterial
{

QT_FORWARD_DECLARE_CLASS(TweaksGLWidget)
QT_FORWARD_DECLARE_CLASS(DoubleSlider)

class TweaksDialog: public QDialog
{
    Q_OBJECT

public:
    explicit TweaksDialog(QWidget* parent=nullptr);
    ~TweaksDialog();

    const QString& get_tweaked_image_path() const;

    void set_clear_color(const wcore::math::vec3& value);
    void set_source_image(const QString& path);
    void reset();

public slots:
    void handle_accept();

private:
    TweaksGLWidget* preview_;
    DoubleSlider* sld_hue_;
    DoubleSlider* sld_saturation_;
    DoubleSlider* sld_value_;
};

} // namespace waterial

#endif
