#ifndef PREVIEW_CONTROLS_H
#define PREVIEW_CONTROLS_H

#include <QGroupBox>

QT_FORWARD_DECLARE_CLASS(QComboBox)

namespace waterial
{

QT_FORWARD_DECLARE_CLASS(PreviewGLWidget)
QT_FORWARD_DECLARE_CLASS(DoubleSlider)

class PreviewControlWidget: public QGroupBox
{
    Q_OBJECT

public:
    explicit PreviewControlWidget(PreviewGLWidget* preview, QWidget* parent = 0);

signals:
    void sig_light_type_changed(int newvalue);

protected slots:
    void handle_light_type_changed(int newvalue);
    void handle_reset_cam_orientation();
    void handle_reset_model_position();

protected:
    QGroupBox* create_general_controls(PreviewGLWidget* preview);
    QGroupBox* create_model_controls(PreviewGLWidget* preview);
    QGroupBox* create_light_controls(PreviewGLWidget* preview);
    QGroupBox* create_camera_controls(PreviewGLWidget* preview);

private:
    QComboBox* combo_light_type_;
    QWidget* point_light_controls_;
    QWidget* dir_light_controls_;

    DoubleSlider* sld_cam_r_;
    DoubleSlider* sld_cam_theta_;
    DoubleSlider* sld_cam_phi_;
    DoubleSlider* sld_mod_x_;
    DoubleSlider* sld_mod_y_;
    DoubleSlider* sld_mod_z_;
};

} // namespace waterial

#endif // PREVIEW_CONTROLS_H
