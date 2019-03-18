#ifndef PREVIEW_CONTROLS_H
#define PREVIEW_CONTROLS_H

#include <QGroupBox>

QT_FORWARD_DECLARE_CLASS(QComboBox)

namespace waterial
{

QT_FORWARD_DECLARE_CLASS(GLWidget)

class PreviewControlWidget: public QGroupBox
{
    Q_OBJECT

public:
    explicit PreviewControlWidget(GLWidget* preview, QWidget* parent = 0);

signals:
    void sig_light_type_changed(int newvalue);

protected slots:
    void handle_light_type_changed(int newvalue);

protected:
    QGroupBox* create_general_controls(GLWidget* preview);
    QGroupBox* create_model_controls(GLWidget* preview);
    QGroupBox* create_light_controls(GLWidget* preview);
    QGroupBox* create_camera_controls(GLWidget* preview);

private:
    QComboBox* combo_light_type_;
    QWidget* point_light_controls_;
    QWidget* dir_light_controls_;
};

} // namespace waterial

#endif // PREVIEW_CONTROLS_H
