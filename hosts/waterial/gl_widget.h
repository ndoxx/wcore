#ifndef GL_WIDGET_H
#define GL_WIDGET_H

#include <QOpenGLWidget>

namespace wcore
{
class Engine;
}

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace medit
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
    float dphi_;
    float dtheta_;
    float dpsi_;
};

} // namespace medit

#endif // GL_WIDGET_H
