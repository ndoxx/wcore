#ifndef GL_WIDGET_H
#define GL_WIDGET_H

#include <QOpenGLWidget>

#include "math3d.h"

namespace wcore
{
class Engine;
class Material;
struct MaterialDescriptor;
}

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace waterial
{

class QtContext;
class GLWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit GLWidget(QWidget* parent = 0);
    ~GLWidget();

    QSize minimumSizeHint() const Q_DECL_OVERRIDE;

    // Global controls
    inline void set_active(bool value) { active_ = value; }
    inline bool get_active() const     { return active_; }

    inline void set_model_rotation(bool value) { rotate_model_ = value; }
    inline void set_model_dphi(float value)    { dphi_ = value; }
    inline void set_model_dtheta(float value)  { dtheta_ = value; }
    inline void set_model_dpsi(float value)    { dpsi_ = value; }

public slots:
    void cleanup();
    void handle_active_changed(int newstate);
    void handle_rotate_changed(int newstate);
    void handle_dphi_changed(double newvalue);
    void handle_dtheta_changed(double newvalue);
    void handle_dpsi_changed(double newvalue);
    void handle_reset_orientation();
    void handle_x_changed(double newvalue);
    void handle_y_changed(double newvalue);
    void handle_z_changed(double newvalue);
    void handle_material_swap(const wcore::MaterialDescriptor& descriptor);

    void handle_light_radius_changed(double newvalue);
    void handle_light_brightness_changed(double newvalue);
    void handle_light_ambient_changed(double newvalue);
    void handle_light_x_changed(double newvalue);
    void handle_light_y_changed(double newvalue);
    void handle_light_z_changed(double newvalue);
    void handle_light_inclination_changed(double newvalue);
    void handle_light_perihelion_changed(double newvalue);
    void handle_light_color_changed(QColor newvalue);
    void handle_light_type_changed(int newvalue);

    void handle_bloom_changed(int newvalue);

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(int width, int height) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;

private:
    wcore::Engine* engine_;
    QtContext* context_;
    QTimer* frame_timer_;

    // Global controls
    bool active_;

    // Model properties
    bool rotate_model_;
    bool reset_orientation_;
    bool show_light_proxy_;
    float dphi_;
    float dtheta_;
    float dpsi_;
    wcore::math::vec3 model_pos_;
    wcore::math::vec3 light_pos_;
    wcore::math::vec3 light_color_;
    float light_brightness_;
    float light_amb_;
    float light_radius_;
    float light_inclination_;
    float light_perihelion_;
    int light_type_;
    wcore::Material* new_material_;
};

} // namespace waterial

#endif // GL_WIDGET_H
