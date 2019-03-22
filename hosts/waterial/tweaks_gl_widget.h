#ifndef TWEAKS_GL_WIDGET_H
#define TWEAKS_GL_WIDGET_H

#include <QString>
#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_0_Core>

#include "math3d.h"

QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)
QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)
QT_FORWARD_DECLARE_CLASS(QOpenGLBuffer)
QT_FORWARD_DECLARE_CLASS(QOpenGLVertexArrayObject)

namespace waterial
{

class TweaksGLWidget: public QOpenGLWidget, protected QOpenGLFunctions_4_0_Core
{
    Q_OBJECT

public:
    explicit TweaksGLWidget(QWidget* parent=nullptr);
    ~TweaksGLWidget();

    QSize minimumSizeHint() const Q_DECL_OVERRIDE;

    inline void set_clear_color(const wcore::math::vec3& value) { clear_color_ = value; }
    void set_source_image(const QString& path);

public slots:
    void handle_hue_changed(double newvalue);
    void handle_saturation_changed(double newvalue);
    void handle_value_changed(double newvalue);

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(int width, int height) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;

private:
    wcore::math::vec3 clear_color_;
    QString source_path_;
    QOpenGLTexture* source_tex_;
    QOpenGLShaderProgram* program_;
    QOpenGLBuffer* vbo_;
    QOpenGLVertexArrayObject* vao_;

    unsigned int attr_position_;
    bool initialized_;

    // uniforms
    float hue_;
    float saturation_;
    float value_;
};

} // namespace waterial

#endif
