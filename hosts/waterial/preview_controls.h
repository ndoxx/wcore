#ifndef PREVIEW_CONTROLS_H
#define PREVIEW_CONTROLS_H

#include <QGroupBox>

namespace waterial
{

QT_FORWARD_DECLARE_CLASS(GLWidget)

class PreviewControlWidget: public QGroupBox
{
    Q_OBJECT

public:
    explicit PreviewControlWidget(GLWidget* preview, QWidget* parent = 0);

protected:
    QGroupBox* create_general_controls(GLWidget* preview);
    QGroupBox* create_model_controls(GLWidget* preview);
    QGroupBox* create_light_controls(GLWidget* preview);
    QGroupBox* create_camera_controls(GLWidget* preview);

};

} // namespace waterial

#endif // PREVIEW_CONTROLS_H
